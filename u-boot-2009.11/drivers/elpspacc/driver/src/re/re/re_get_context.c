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

int re_get_spacc_context(re_device *re, int re_ctx)
{
    if (re_ctx < 0 || re_ctx >= re->config.num_ctx) {
       ELPHW_PRINT ("re_get_spacc_context: Invalid Context");
       return CRYPTO_INVALID_CONTEXT;
    }

    return re->re_contexts[re_ctx].spacc_handle;
 }

int re_get_context_ex (re_device * re, int ctxid, re_callback cb, void *cbdata)
{
   int i, j;
   unsigned long flag;

   PDU_LOCK(&re->spacc->lock, flag);

   // find a RE context to use
   for (j = 0; j < SPACC_MAX_JOBS; j++) {
      if (re->re_contexts[j].spacc_handle == -1) { break; }
   }
   if (j == SPACC_MAX_JOBS) {
      PDU_UNLOCK(&re->spacc->lock, flag);
      return -1;
   }

   // allocate a spacc context
   i = spacc_ctx_request (re->spacc, ctxid, 1);
   if (i < 0) {
      PDU_UNLOCK(&re->spacc->lock, flag);
      return -1;
   }


   // TODO: if we are not finding a new one we need a new RE context but we bind it to this handle
   if (re_init_context_ex (re, j, i) != CRYPTO_OK) {
      PDU_UNLOCK(&re->spacc->lock, flag);
      return CRYPTO_FAILED;
   }

   re->re_contexts[j].cb     = cb;
   re->re_contexts[j].cbdata = cbdata;

   PDU_UNLOCK(&re->spacc->lock, flag);
   return j;
}


/*
 * Get an initialized context.. Will return CRYPTO_FAILED
 * if there are no available contexts... or if the context
 * is invalid,or it will return the context number if
 * it succeeded.
 */
int re_get_context (re_device * re, re_callback cb, void *cbdata)
{
   return re_get_context_ex(re, -1, cb, cbdata);
}
