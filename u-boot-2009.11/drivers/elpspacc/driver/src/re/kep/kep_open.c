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

int kep_open (kep_device * kep, int op, int option, kep_callback cb, void *cbdata)
{
   int handle;
   unsigned long flag;

   PDU_LOCK(&kep->spacc->lock, flag);
   handle = spacc_ctx_request (kep->spacc, -1, ((op == KEP_TLS_PRF) || (op == KEP_TLS_SIGN)) ? 2 : 1);
   PDU_UNLOCK(&kep->spacc->lock, flag);
   if (handle < 0) {
      ELPHW_PRINT ("kep_open::Failed to allocate a spacc context handle\n");
      return CRYPTO_FAILED;
   }
   kep->ctx[handle].op = op;
   kep->ctx[handle].option = option;
   kep->ctx[handle].job_id = -1;
   kep->ctx[handle].job_done = 0;
   
   kep->ctx[handle].cb     = cb;
   kep->ctx[handle].cbdata = cbdata;

   return handle;
}
