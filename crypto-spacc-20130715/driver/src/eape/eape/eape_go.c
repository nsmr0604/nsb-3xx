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

#include "eape.h"

static int eape_go_ex(eape_device *eape, int handle, int direction, pdu_ddt *src, pdu_ddt *dst, eape_callback cb, void *cb_data)
{
   int err, j;
   uint32_t offset;

   if (handle < 0 || handle > eape->num_ctx || eape->ctx[handle].allocated == 0) {
      return -1;
   }

   if (!direction) {
      offset = 0x20;
   } else {
      offset = 0;
   }

   /* is fifo is full */
   if (EAPE_STAT_CMD_FIFO_FULL(pdu_io_read32(eape->regmap + offset + EAPE_REG_OUT_STAT))) {
      return EAPE_FIFO_FULL;
   }

   err = 0;

   /* we're running the job now */

   // find a slot for the job
   for (j = 0; j < MAXJOBS; j++) {
      if (eape->ctx[handle].done[j] == 1) {
         break;
      }
   }
   if (j == MAXJOBS) {
      err = EAPE_JOBS_FULL;
      goto ERR;
   }

   // have slot, allocate handle if needed
   eape->ctx[handle].job_cnt++;
   eape->ctx[handle].done[j]   = 0;
   eape->ctx[handle].cb[j]     = cb;
   eape->ctx[handle].cbdata[j] = cb_data;

#if 0
{
   int x;
   printk("SRC DDT == %08lx\n", src->phys);
   for (x = 0; x <= src->idx; x++) {
      printk("SRC DDT[%2d] == %08lx\n", 2*x+0, src->virt[2*x+0]);
      printk("SRC DDT[%2d] == %08lx\n", 2*x+1, src->virt[2*x+1]);
   }
   printk("DST DDT == %08lx\n", dst->phys);
   for (x = 0; x <= dst->idx; x++) {
      printk("DST DDT[%2d] == %08lx\n", 2*x+0, dst->virt[2*x+0]);
      printk("DST DDT[%2d] == %08lx\n", 2*x+1, dst->virt[2*x+1]);
   }
   printk("SAI == 0x%08zx\n", eape->sa_ptr_phys + SA_SIZE*handle);
}
#endif


   // write out engine
   pdu_io_cached_write32(eape->regmap + offset + EAPE_REG_OUT_SRC_PTR, (uint32_t) src->phys, &(eape->cache[direction].src_ptr));
   pdu_io_cached_write32(eape->regmap + offset + EAPE_REG_OUT_DST_PTR, (uint32_t) dst->phys, &(eape->cache[direction].dst_ptr));
   pdu_io_cached_write32(eape->regmap + offset + EAPE_REG_OUT_OFFSET, 0, &(eape->cache[direction].offset));

   err = pdu_io_read32(eape->regmap + offset + EAPE_REG_OUT_ID);
   err = ((err - 0) & 0xFF) | ((!!direction)<<8);
   eape->ctx[handle].swid[j] = err;
   eape->swid[eape->ctx[handle].swid[j]] = (handle<<16)|(j);

   pdu_io_write32(eape->regmap + offset + EAPE_REG_OUT_SAI, eape->sa_ptr_phys + SA_SIZE * handle);
ERR:
   return err;
}


int eape_go(eape_device *eape, int handle, int direction, pdu_ddt *src, pdu_ddt *dst, eape_callback cb, void *cb_data)
{
   int err;
   unsigned long flag;

   PDU_LOCK(&eape->lock, flag);
   err = eape_go_ex(eape, handle, direction, src, dst, cb, cb_data);
   PDU_UNLOCK(&eape->lock, flag);
   return err;
}
