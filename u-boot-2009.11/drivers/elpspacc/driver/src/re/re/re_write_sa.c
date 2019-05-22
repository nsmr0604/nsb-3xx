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
 *  Writes SA data from a buffer to the SA of the given Context. This can be used
 *  to recontruct a context from earlier.
 */

int re_write_sa (re_device * re, int handle, unsigned char * sa, uint32_t bufflength)
{
   elp_re_ctx *ctx;
   if (handle < 0 || handle >= re->config.num_ctx) {
      return CRYPTO_INVALID_CONTEXT;
   }

   ctx = &re->re_contexts[handle];

   if (bufflength < 1024) {
      return CRYPTO_INVALID_SIZE;
   }
   pdu_to_dev32_little(ctx->sa, sa, 1024/4);
   return CRYPTO_OK;
}
