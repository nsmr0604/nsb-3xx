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

#include "elpea.h"

int ea_pop_packet(ea_device *ea)
{
  uint32_t len, retcode, sw_id, sttl, status, x, y;
  unsigned long flags;
  int      jobs;
  ea_callback cb;
  void *cb_data;

  PDU_LOCK(&ea->spacc->lock, flags);

   for (;;) {
      if (ea->spacc->config.pdu_version >= 0x25) {
         jobs = EA_FIFO_STAT_STAT_CNT(pdu_io_read32(ea->regmap + EA_FIFO_STAT));
      } else {
         jobs = EA_FIFO_STAT_STAT_CNT_V24(pdu_io_read32(ea->regmap + EA_FIFO_STAT));
      }
      if (!jobs) {
         break;
      }

     while (jobs-- > 0) {
        pdu_io_write32(ea->regmap + EA_POP, 1);
        status  = pdu_io_read32(ea->regmap + EA_STATUS);
        len     = EA_STATUS_LEN(status);
        sw_id   = EA_STATUS_SW_ID(status);
        retcode = EA_STATUS_RET_CODE(status);
        sttl    = EA_STATUS_STTL(status);
   //     printk(KERN_DEBUG "SWID popped off FIFO swid==%zu, status==%zx, len==%zu retcode==%zu sttl==%zu\n", sw_id, status, len, retcode, sttl);

        // look up the sw_id
        x = ea->swid[sw_id] >> 16;    // handle
        y = ea->swid[sw_id] & 0xFFFF; // job slot inside context
        if (!((x < ea->num_ctx) && (y < MAXJOBS) && (ea->ctx[x].swid[y] == sw_id))) {
           ELPHW_PRINT("ea_pop_packets::Invalid SW_ID returned by SPAcc-EA %08zx\n", sw_id);
           PDU_UNLOCK(&ea->spacc->lock, flags);
           return -1;
        }
        ea->swid[sw_id] = 0xFFFFFFFF;  // reset lookup so it won't match anything again

        /* Store callback information and mark job as done. */
        cb      = ea->ctx[x].cb[y];
        cb_data = ea->ctx[x].cbdata[y];

        ea->ctx[x].cb[y]   = NULL;
        ea->ctx[x].done[y] = 1;
        --(ea->ctx[x].job_cnt);

        if ((ea->ctx[x].dismiss || ea->ctx[x].lock_ctx == 0) && ea->ctx[x].job_cnt == 0) {
           // free spacc handle
           spacc_ctx_release (ea->spacc, ea->ctx[x].spacc_handle);
           ea->ctx[x].spacc_handle = -1;

           if (ea->ctx[x].dismiss) {
              ea->ctx[x].allocated = 0;
           }
        }

        if (cb) {
           PDU_UNLOCK(&ea->spacc->lock, flags);
           cb(ea, cb_data, len, retcode, sttl, sw_id);
           PDU_LOCK(&ea->spacc->lock, flags);
        }
     }
  }
  // are there any jobs pending in the job buffer...
  if (ea->ea_job_buf_use) {
    ea->ea_job_buf_use = 0;
    for (x = 0; x < EA_MAX_BACKLOG; x++) {
      if (ea->ea_job_buf[x].active) {
        y = ea_go_ex(ea, 0, ea->ea_job_buf[x].handle, ea->ea_job_buf[x].direction, ea->ea_job_buf[x].src, ea->ea_job_buf[x].dst, ea->ea_job_buf[x].cb, ea->ea_job_buf[x].cb_data);
        if (y != EA_FIFO_FULL) {
           ea->ea_job_buf[x].active = 0;
        } else {
           ea->ea_job_buf_use |= 1;
        }
      }
    }
  }
  PDU_UNLOCK(&ea->spacc->lock, flags);
  return 0;
}

