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
 *  Release the context supplied in handle, and clean up all memory that has
 *  been used
 */
int re_release_context (re_device * re, int handle)
{
   elp_re_ctx *ctx;
   if (handle < 0 || handle >= re->config.num_ctx) {
      return CRYPTO_INVALID_CONTEXT;
   }

   ctx = &re->re_contexts[handle];
   if (ctx->spacc_handle >= 0) {
      unsigned long flag;

      PDU_LOCK(&re->spacc->lock, flag);
      spacc_ctx_release (re->spacc, ctx->spacc_handle);
      ctx->spacc_handle = -1;
      PDU_UNLOCK(&re->spacc->lock, flag);
   }

   return CRYPTO_OK;
}
