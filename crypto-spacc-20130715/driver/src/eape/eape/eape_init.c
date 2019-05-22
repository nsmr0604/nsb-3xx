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

int eape_init(eape_device *eape, void *regmap, int no_ctx)
{
   int x;

   eape->regmap = regmap;

   eape->num_ctx        = no_ctx;
   eape->sa_ptr_mem_req = SA_SIZE * no_ctx;

   eape->ctx = pdu_malloc(sizeof(*(eape->ctx)) * no_ctx);
   if (!eape->ctx) {
      ELPHW_PRINT("Out of memory\n");
      return -1;
   }
   memset(eape->ctx, 0, sizeof(*(eape->ctx)) * no_ctx);

   memset(&(eape->cache), 0, sizeof (eape->cache));
   pdu_io_write32(regmap + EAPE_REG_OUT_SRC_PTR, 0);
   pdu_io_write32(regmap + EAPE_REG_OUT_DST_PTR, 0);
   pdu_io_write32(regmap + EAPE_REG_OUT_OFFSET, 0);
   pdu_io_write32(regmap + EAPE_REG_IN_SRC_PTR, 0);
   pdu_io_write32(regmap + EAPE_REG_IN_DST_PTR, 0);
   pdu_io_write32(regmap + EAPE_REG_IN_OFFSET, 0);

   eape->stat_cnt   = 1;
   eape->fifo_depth = 4;

   for (x = 0; x < 512; x++) {
      eape->swid[x] = 0xFFFFFFFF;
   }

   PDU_INIT_LOCK(&eape->lock);

   return 0;
}

