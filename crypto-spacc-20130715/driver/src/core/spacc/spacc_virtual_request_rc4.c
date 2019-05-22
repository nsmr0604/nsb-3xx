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

int spacc_virtual_request_rc4 (spacc_device * spacc)
{
   volatile int timeout = 1000000;
   pdu_io_write32 (spacc->regmap + SPACC_REG_VIRTUAL_RC4_KEY_RQST, 1);
   while (pdu_io_read32 (spacc->regmap + SPACC_REG_VIRTUAL_RC4_KEY_GNT) != 1) {
      if (--timeout == 0) {
         ELPHW_PRINT ("Failed to request RC4 key context, timeout\n");
         return CRYPTO_FAILED;
      }
   }
   return CRYPTO_OK;
}
