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

int spacc_request_hsm_semaphore(spacc_device *spacc)
{
   unsigned long timeout, v;
   timeout = 100000UL;
   pdu_io_write32(spacc->regmap + SPACC_REG_HSM_CMD_REQ, 1); // request SP
   while (--timeout && !((v = pdu_io_read32(spacc->regmap + SPACC_REG_HSM_CMD_GNT)) & 1));
   if (!timeout) {
      ELPHW_PRINT("spacc_packet_enqueue_ddt::Timeout on requesting CTRL register from HSM shared\n");
      return CRYPTO_FAILED;
   }
   return CRYPTO_OK;
}

void spacc_free_hsm_semaphore(spacc_device *spacc)
{
   pdu_io_write32(spacc->regmap + SPACC_REG_HSM_CMD_REQ, 0);
}

