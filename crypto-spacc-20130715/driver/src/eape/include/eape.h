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

#ifndef _ELPEAPE_H_
#define _ELPEAPE_H_

#include "elppdu.h"
#include "eapehw.h"

// max jobs per context 
#define MAXJOBS 64

typedef void (*eape_callback)(void *eape_dev, void *data, uint32_t payload_len, uint32_t retcode, uint32_t swid);

typedef struct {
   uint32_t spi,
            flags,
            seqnum[2],
            hard_ttl[2],
            soft_ttl[2];
   unsigned char 
            alg,
            armask[8],
            ckey[32],
            csalt[4],
            mackey[64];
} eape_sa;                        

typedef struct {
   int dismiss,
       job_cnt,
       allocated,
       swid[MAXJOBS],
       done[MAXJOBS];
   
   eape_callback cb[MAXJOBS];
   void       *cbdata[MAXJOBS];   
} eape_ctx;

typedef struct {
  void *regmap;

  eape_ctx *ctx;
  int     num_ctx,
          fifo_depth,
          stat_cnt;
  
  void           *sa_ptr_virt;
  PDU_DMA_ADDR_T  sa_ptr_phys;
  
  PDU_LOCK_TYPE   lock;
 
  uint32_t           swid[512];
  
  size_t sa_ptr_mem_req;
  
  struct {
     uint32_t src_ptr,
              dst_ptr,
              offset;
  } cache[2];            
  
} eape_device;

int eape_init(eape_device *eape, void *regmap, int no_ctx);
int eape_deinit(eape_device *eape);

int eape_open(eape_device *eape);
int eape_build_sa(eape_device *eape, int handle, eape_sa *sa);
int eape_go(eape_device *eape, int handle, int direction, pdu_ddt *src, pdu_ddt *dst, eape_callback cb, void *cb_data);
int eape_close(eape_device *eape, int handle);
int eape_done(eape_device *eape, int handle, int swid);

int eape_pop_packet(eape_device *eape);

// kernel
eape_device *eape_get_device(void);

#endif
