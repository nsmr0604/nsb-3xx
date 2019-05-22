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


#include "elpre.h"
#include "elprehw.h"


/*
 * Start the record engine, must give the type of operation(0-4) and the length of the data
 */
int re_start_operation_ex (re_device * re, int handle, pdu_ddt * src_ddt, pdu_ddt * dst_ddt, uint32_t srcoff, uint32_t dstoff, uint32_t paylen, uint32_t type)
{
   //Make sure the the context is valid, and if it is have ctx point to it
   uint32_t temp, swid;
   elp_re_ctx *ctx;
   unsigned long lock_flag;

   if (handle < 0 || handle >= re->config.num_ctx) {
      return CRYPTO_INVALID_CONTEXT;
   }
   ctx = &re->re_contexts[handle];

   PDU_LOCK(&re->lock, lock_flag);
   if (RE_FIFO_CMD_FULL (pdu_io_read32 (re->regmap + RE_FIFO_STAT))) {
      PDU_UNLOCK(&re->lock, lock_flag);
      return CRYPTO_FIFO_FULL;
   }

   temp = 0x00;
   temp |= type;
   temp |= (ctx->spacc_handle << 16) & 0xFF0000;
   temp |= RE_LOAD_STORE_SA;

   pdu_io_cached_write32 (re->regmap + RE_SA_PTR,  (uint32_t)ctx->samap,    &re->cache.sa_ptr);
   pdu_io_cached_write32 (re->regmap + RE_SRC_PTR, (uint32_t)src_ddt->phys, &re->cache.src_ptr);
   pdu_io_cached_write32 (re->regmap + RE_DST_PTR, (uint32_t)dst_ddt->phys, &re->cache.dst_ptr);
   swid = pdu_io_read32 (re->regmap + RE_SW_CTRL) & 0xFF;
   re->jobid_to_ctx[swid] = handle;
   ctx->curjob_swid = swid;
   pdu_io_cached_write32 (re->regmap + RE_OFFSET, (srcoff<<0)|(dstoff<<16), &re->cache.offset);
   pdu_io_cached_write32 (re->regmap + RE_LEN, paylen & 0x0000FFFF,         &re->cache.len);
   pdu_io_write32 (re->regmap + RE_CTRL, temp);
   PDU_UNLOCK(&re->lock, lock_flag);

   return swid;
}

int re_start_operation (re_device * re, int handle, pdu_ddt * src_ddt, pdu_ddt * dst_ddt, uint32_t type)
{
   return re_start_operation_ex(re, handle, src_ddt, dst_ddt, 0, 0, src_ddt->len, type);
}
