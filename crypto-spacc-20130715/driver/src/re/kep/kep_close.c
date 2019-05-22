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
#include "elpkep.h"

int kep_close (kep_device * kep, int handle)
{
   unsigned long flag;

   if (kep_is_valid (kep, handle)) {
      ELPHW_PRINT ("kep_close::Invalid handle specified\n");
      return CRYPTO_FAILED;
   }

   kep->ctx[handle].job_id = -1;

   PDU_LOCK(&kep->spacc->lock, flag);
   spacc_ctx_release (kep->spacc, handle);
   PDU_UNLOCK(&kep->spacc->lock, flag);
   return CRYPTO_OK;
}
