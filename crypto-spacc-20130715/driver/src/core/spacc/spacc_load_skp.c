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

int spacc_load_skp(spacc_device *spacc, uint32_t *key, int keysz, int idx, int alg, int mode, int size, int enc, int dec)
{
   uint32_t t;
   unsigned long lock_flags;

   // only secure mode can use it
   if (spacc->config.is_secure_port == 0) {
      ELPHW_PRINT("spacc_load_skp:: Only the secure port can use the SKP\n");
      return CRYPTO_FAILED;
   }

   if (keysz*4 > spacc->config.sec_ctx_page_size) {
      ELPHW_PRINT("spacc_load_skp:: Invalid key size for secure key\n");
      return CRYPTO_FAILED;
   }

   if (idx < 0 || idx > spacc->config.num_sec_ctx) {
      ELPHW_PRINT("spacc_load_skp:: Invalid CTX id specified (out of range)\n");
      return CRYPTO_FAILED;
   }

   PDU_LOCK(&spacc->lock, lock_flags);

   // wait for busy to clear
   t = 100000UL;
   while(--t && (pdu_io_read32(spacc->regmap + SPACC_REG_SK_STAT) & SPACC_SK_STAT_BUSY));
   if (!t) {
      ELPHW_PRINT("spacc_load_skp:: SK_STAT never cleared\n");
      PDU_UNLOCK(&spacc->lock, lock_flags);
      return CRYPTO_FAILED;
   }

   // copy key
   pdu_to_dev32(spacc->regmap + SPACC_REG_SK_KEY, key, keysz);

   // set reg
   t = (idx << _SPACC_SK_LOAD_CTX_IDX) |
       (alg << _SPACC_SK_LOAD_ALG)     |
       (mode << _SPACC_SK_LOAD_MODE)   |
       (size << _SPACC_SK_LOAD_SIZE)   |
       (enc << _SPACC_SK_LOAD_ENC_EN)  |
       (dec << _SPACC_SK_LOAD_DEC_EN);

   pdu_io_write32(spacc->regmap + SPACC_REG_SK_LOAD, t);

   PDU_UNLOCK(&spacc->lock, lock_flags);

   return CRYPTO_OK;
}


