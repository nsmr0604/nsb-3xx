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
#include "elpkep.h"

int kep_load_keys (kep_device * kep, int handle, void *s1, uint32_t s1len, void *s2, uint32_t s2len)
{
   spacc_device *spacc;

   if (kep_is_valid (kep, handle)) {
      ELPHW_PRINT ("kep_load_keys::Invalid handle specified\n");
      return CRYPTO_FAILED;
   }

   spacc = kep->spacc;
   if (!spacc) {
      ELPHW_PRINT ("kep_load_keys::SPACC module not initialized\n");
      return CRYPTO_FAILED;
   }

   if ((((s1len+3)>>2)<<2) > spacc->config.hash_page_size) {
      ELPHW_PRINT ("kep_load_keys::Invalid hash key size\n");
      return CRYPTO_FAILED;
   }

   if ((((s2len+3)>>2)<<2) > spacc->config.hash_page_size) {
      ELPHW_PRINT ("kep_load_keys::Invalid hash key size\n");
      return CRYPTO_FAILED;
   }

   pdu_to_dev32_s(spacc->ctx[handle].hash_key, s1, (s1len + 3) >> 2, spacc_endian);
   pdu_io_write32 (spacc->regmap + SPACC_REG_KEY_SZ, SPACC_SET_HASH_KEY_SZ (s1len) | SPACC_SET_KEY_CTX (handle));
   if (s2) {
      if ((handle + 1) >= spacc->config.num_ctx) {
         ELPHW_PRINT ("kep_load_keys::Invalid handle specified for 2nd key\n");
         return CRYPTO_FAILED;
      }
      pdu_to_dev32_s(spacc->ctx[handle+1].hash_key, s2, (s2len + 3) >> 2, spacc_endian);
      pdu_io_write32 (spacc->regmap + SPACC_REG_KEY_SZ, SPACC_SET_HASH_KEY_SZ (s2len) | SPACC_SET_KEY_CTX (handle + 1));
   }
   return 0;
}
