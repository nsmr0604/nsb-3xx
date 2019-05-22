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
 *  Resets the SA
 */
int re_reset_sa (re_device * re, int handle, uint32_t version)
{
   elp_re_ctx *ctx;
   if (handle < 0 || handle >= re->config.num_ctx) {
      return CRYPTO_INVALID_CONTEXT;
   }

   ctx = &re->re_contexts[handle];
   memset(ctx->sa, 0x00, RE_SA_SIZE);
   ctx->sa[RE_SA_FLAGS] = RE_SA_MAJOR_VERSION;  //We set the version in the SA to 3.(version)
   ctx->sa[RE_SA_FLAGS] |= (version & 0x0F) << 4;
   return CRYPTO_OK;
}
