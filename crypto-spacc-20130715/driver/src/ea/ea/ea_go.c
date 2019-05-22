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

static int cmd_full(ea_device *ea, int fifo)
{
   if (!fifo) {
      return EA_FIFO_STAT_CMD_FULL(pdu_io_read32(ea->regmap + EA_FIFO_STAT));
   } else {
      return EA_FIFO_STAT_CMD1_FULL(pdu_io_read32(ea->regmap + EA_FIFO_STAT));
   }
}

int ea_go_ex(ea_device *ea, int use_job_buf, int handle, int direction, pdu_ddt *src, pdu_ddt *dst, ea_callback cb, void *cb_data)
{
   int err, j, fifo;
   unsigned first;

   if (handle < 0 || handle > ea->num_ctx || ea->ctx[handle].allocated == 0) {
      return -1;
   }

   fifo = direction ? ea->config.tx_fifo : ea->config.rx_fifo;

   if (cmd_full(ea, fifo)) {
      if (ea->config.tx_fifo != ea->config.rx_fifo) {
         fifo ^= 1;
         if (cmd_full(ea, fifo)) {
            goto add_to_buffer;
         }
      } else {
         goto add_to_buffer;
      }
   }

   /* we're running the job now */

   // find a slot for the job
   for (j = 0; j < MAXJOBS; j++) {
      if (ea->ctx[handle].done[j] == 1) {
         break;
      }
   }
   if (j == MAXJOBS) {
      err = EA_JOBS_FULL;
      goto ERR;
   }

   // have slot, allocate handle if needed
   first = 0;
   if (ea->ctx[handle].spacc_handle == -1) {
      first = 1;
      ea->ctx[handle].spacc_handle = spacc_ctx_request(ea->spacc, -1, 1);
      if (ea->ctx[handle].spacc_handle < 0) {
         return EA_NO_CONTEXT;
      }
   }

   ea->ctx[handle].job_cnt++;
   ea->ctx[handle].done[j]   = 0;
   ea->ctx[handle].cb[j]     = cb;
   ea->ctx[handle].cbdata[j] = cb_data;

   // write out engine
   if (ea->spacc->config.dma_type == SPACC_DMA_DDT) {
      pdu_io_cached_write32 (ea->regmap + EA_SRC_PTR, (uint32_t) src->phys, &(ea->cache.src_ptr));
      pdu_io_cached_write32 (ea->regmap + EA_DST_PTR, (uint32_t) dst->phys, &(ea->cache.dst_ptr));
   } else if (ea->spacc->config.dma_type == SPACC_DMA_LINEAR) {
      pdu_io_cached_write32 (ea->regmap + EA_SRC_PTR, (uint32_t) src->virt[0], &(ea->cache.src_ptr));
      pdu_io_cached_write32 (ea->regmap + EA_DST_PTR, (uint32_t) dst->virt[0], &(ea->cache.dst_ptr));
   } else {
      err = -1;
      goto ERR;
   }

   pdu_io_cached_write32(ea->regmap + EA_OFFSET, 0, &(ea->cache.offset));
   pdu_io_cached_write32(ea->regmap + EA_SA_PTR, ea->sa_ptr_phys + SA_SIZE * handle, &(ea->cache.sa_ptr));

   err = ea->ctx[handle].swid[j] = pdu_io_read32(ea->regmap + EA_SW_CTRL);
   ea->swid[ea->ctx[handle].swid[j]] = (handle<<16)|(j);

   pdu_io_write32(ea->regmap + EA_CTRL, EA_CTRL_FIFO_SEL(fifo)|EA_CTRL_CMD(direction)|EA_CTRL_CTX_ID(ea->ctx[handle].spacc_handle)|EA_CTRL_LD_CTX(first));

ERR:
   return err;

add_to_buffer:
   if (use_job_buf) {
      // no more room in FIFO so add to job queue
      // find room in job buff
      for (j = 0; j < EA_MAX_BACKLOG; j++) {
         if (ea->ea_job_buf[j].active == 0) {
            // add it here
            ea->ea_job_buf[j] = (struct ea_job_buf) {.handle = handle, .direction = direction, .src = src, .dst = dst, .cb = cb, .cb_data = cb_data, .active = 1 };
            ea->ea_job_buf_use |= 1;
            return 256;
         }
      }
   }
   return EA_FIFO_FULL;
}


int ea_go(ea_device *ea, int handle, int direction, pdu_ddt *src, pdu_ddt *dst, ea_callback cb, void *cb_data)
{
   int err;
   unsigned long flag;

   PDU_LOCK(&ea->spacc->lock, flag);
   err = ea_go_ex(ea, 1, handle, direction, src, dst, cb, cb_data);
   PDU_UNLOCK(&ea->spacc->lock, flag);
   return err;
}

