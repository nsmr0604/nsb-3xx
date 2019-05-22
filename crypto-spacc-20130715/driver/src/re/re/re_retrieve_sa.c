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
 *  Retrieves the SA from the context. This can be used to reconstruct the context
 *  at a later time. The sa is stored in sabuff, which must be at least 1024 bytes
 */
int re_retrieve_sa (re_device * re, int handle, unsigned char * sabuff, uint32_t bufflength)
{
   elp_re_ctx *ctx;
   if (handle < 0 || handle >= re->config.num_ctx) {
      return CRYPTO_INVALID_CONTEXT;
   }
   ctx = &re->re_contexts[handle];

   if (bufflength < 1024) {
      return CRYPTO_INVALID_SIZE;
   }

   pdu_from_dev32_little(sabuff, ctx->sa, 1024/4);
   return CRYPTO_OK;
}
