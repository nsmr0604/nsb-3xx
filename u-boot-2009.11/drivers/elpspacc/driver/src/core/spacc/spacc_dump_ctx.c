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

#include "elpspacc.h"

void spacc_dump_ctx(spacc_device *spacc, int ctx)
{
   uint32_t buf[256], x;

   if (ctx < 0 || ctx > SPACC_MAX_JOBS) {
      return;
   }

   ELPHW_PRINT("Dumping Cipher Context %d\n00000: ", ctx);
   pdu_from_dev32(buf, spacc->regmap + SPACC_CTX_CIPH_KEY + (spacc->config.ciph_page_size>>2) * ctx, spacc->config.ciph_page_size>>2);

   for (x = 0; x < spacc->config.ciph_page_size>>2;) {
       ELPHW_PRINT("%08lx ", (unsigned long)buf[x]);
       if (!(++x & 3) && x != (spacc->config.ciph_page_size>>2)) {
          ELPHW_PRINT("\n%05d: ", x);
       }
   }
   ELPHW_PRINT("\n");

   ELPHW_PRINT("Dumping Hash Context %d\n00000: ", ctx);
   pdu_from_dev32(buf, spacc->regmap + SPACC_CTX_HASH_KEY + (spacc->config.hash_page_size>>2) * ctx, spacc->config.hash_page_size>>2);

   for (x = 0; x < spacc->config.hash_page_size>>2;) {
       ELPHW_PRINT("%08lx ", (unsigned long)buf[x]);
       if (!(++x & 3) && x != (spacc->config.hash_page_size>>2)) {
          ELPHW_PRINT("\n%05d: ", x);
       }
   }
   ELPHW_PRINT("\n");
}
