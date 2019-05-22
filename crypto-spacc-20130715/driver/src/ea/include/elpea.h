/*
 * Copyright (c) 2013 Elliptic Technologies Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _ELPEA_H_
#define _ELPEA_H_

#include "elppdu.h"
#include "elpspacc.h"
#include "elpeahw.h"

// operating modes
enum {
   EA_OP_MODE_FULL_DUPLEX_FAVTX, // Favour outbound in largest CMD FIFO, inbound in smallest
   EA_OP_MODE_FULL_DUPLEX_FAVRX, // Favour inbound in largest CMD FIFO, outbound in smallest
};

// max jobs per context
#define MAXJOBS 64

#define EA_MAX_BACKLOG 32

typedef void (*ea_callback)(void *ea_dev, void *data, uint32_t payload_len, uint32_t retcode, uint32_t sttl, uint32_t swid);

typedef struct {
   uint32_t ctrl,
            spi,
            seqnum[2],
            hard_ttl[2],
            soft_ttl[2];
   unsigned char
            armask[32],
            ckey[32],
            csalt[4],
            mackey[64];
} ea_sa;

typedef struct {
   int spacc_handle,
       job_cnt,
       dismiss,
       lock_ctx,
       allocated,
       swid[MAXJOBS],
       done[MAXJOBS];

   ea_callback cb[MAXJOBS];
   void       *cbdata[MAXJOBS];
} ea_ctx;

typedef struct {
  uint32_t idle, active, total;
} ea_clk;

typedef struct {
  spacc_device *spacc;
  void *regmap;

  struct ea_config {
     uint32_t cmd0_depth,
              cmd1_depth,
              sp_ddt_cnt,
              ah_enable,
              ipv6,
              ar_win_size,
              stat_depth,
              ideal_stat_limit,
              max_stat_limit,
              op_mode,
              tx_fifo,
              rx_fifo,
              cur_depth;
  } config;

  struct ea_job_buf {
      int active;

      int handle, direction;
      pdu_ddt *src, *dst;
      ea_callback cb;
      void *cb_data;
  } ea_job_buf[EA_MAX_BACKLOG];
  int ea_job_buf_use;

  ea_ctx *ctx;
  int     num_ctx;

  void           *sa_ptr_virt;
  PDU_DMA_ADDR_T  sa_ptr_phys;

  PDU_LOCK_TYPE   lock;

  uint32_t        swid[256];

  size_t sa_ptr_mem_req;

  struct {
     uint32_t src_ptr,
              dst_ptr,
              offset,
              sa_ptr;
  } cache;

#ifdef EA_PROF
  struct {
      uint32_t dc[128],
               cmd;
  } stats;
#endif
} ea_device;

int ea_init(ea_device *ea, spacc_device *spacc, void *regmap, int no_ctx);
int ea_deinit(ea_device *ea);

int ea_open(ea_device *ea, int lock_ctx);
int ea_build_sa(ea_device *ea, int handle, ea_sa *sa);
int ea_go(ea_device *ea, int handle, int direction, pdu_ddt *src, pdu_ddt *dst, ea_callback cb, void *cb_data);
int ea_go_ex(ea_device *ea, int use_job_buf, int handle, int direction, pdu_ddt *src, pdu_ddt *dst, ea_callback cb, void *cb_data); /* UNLOCKED, DON'T CALL OUTSIDE SDK */
int ea_close(ea_device *ea, int handle);
int ea_done(ea_device *ea, int handle, int swid);
int ea_set_mode(ea_device *ea, int mode);

int ea_pop_packet(ea_device *ea);

// kernel
ea_device *ea_get_device(void);


#endif
