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


int re_set_next_write_sequence_number (re_device * re, int handle, unsigned char * sequence_number, uint32_t length)
{
   int stateoffset;
   elp_re_ctx *ctx;
   if (handle < 0 || handle >= re->config.num_ctx) {
      return CRYPTO_INVALID_CONTEXT;
   }

   ctx = &re->re_contexts[handle];

   if (sequence_number == NULL) {
      return CRYPTO_NOT_INITIALIZED;
   }

   if (length > RE_SA_SEQ_SIZE || length < 0) {
      return CRYPTO_INVALID_SEQUENCE;
   }

   stateoffset = ((ctx->sa[RE_SA_FLAGS] & RE_SA_FLAG_WRITE_PAGE) ? RE_SA_WRITE_STATE_0 : RE_SA_WRITE_STATE_1);

   pdu_to_dev32_big (ctx->sa + stateoffset + RE_SA_STATE_SEQ_NUM, sequence_number, (length+3)/4);
   memset (ctx->sa + stateoffset + RE_SA_STATE_SEQ_NUM + length, 0x00, RE_SA_SEQ_SIZE - length);

   return CRYPTO_OK;
}
