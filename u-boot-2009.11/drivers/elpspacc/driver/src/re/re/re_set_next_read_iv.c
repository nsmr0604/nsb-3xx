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
 * Set the SA's next read IV)
 */
int re_set_next_read_iv (re_device * re, int handle, unsigned char * iv, uint32_t length)
{
   int stateoffset;
   elp_re_ctx *ctx;
   if (handle < 0 || handle >= re->config.num_ctx) {
      return CRYPTO_INVALID_CONTEXT;
   }
   ctx = &re->re_contexts[handle];

   if (length > RE_SA_IVSIZE || length < 0) {   //Make sure the length is valid
      return CRYPTO_INVALID_IV_SIZE;
   }
   if (iv == NULL) {
      return CRYPTO_NOT_INITIALIZED;
   }

   stateoffset = ((ctx->sa[RE_SA_FLAGS] & RE_SA_FLAG_READ_PAGE) ? RE_SA_READ_STATE_0 : RE_SA_READ_STATE_1);

   pdu_to_dev32_little (ctx->sa + stateoffset + RE_SA_STATE_IV, iv, (length+3)/4);
   memset (ctx->sa + stateoffset + RE_SA_STATE_IV + length, 0x00, RE_SA_IVSIZE - length); //Set the "rest" of the IV to 0's
   return CRYPTO_OK;

}
