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
 * Wait for the current operation to finish, if there is no operation running or
 * finished, this will never return
 */
int re_finish_operation (re_device * re, int handle, uint32_t * length, int * id)
{
   //Make sure the the context is vaid, and if it is have ctx point to it
   elp_re_ctx *ctx;
   if (handle < 0 || handle >= re->config.num_ctx) {
      ELPHW_PRINT ("re_finish_operation: Invalid Context");
      return CRYPTO_INVALID_CONTEXT;
   }
   ctx = &re->re_contexts[handle];

   // we no longer call dequeue, only the IRQ does that
   if (ctx->jobdone == 0) {
      return CRYPTO_INPROGRESS;
   }

   ctx->jobdone = 0;            //We finished the job, or error'd out
   if (ctx->ret_stat != CRYPTO_OK) {
      *length = 0;
      *id = ctx->curjob_swid;
      return ctx->ret_stat;
   }

   if (*length < ctx->ret_size) {
      ELPHW_PRINT("re_finish_operation::Invalid length. SPAcc-RE specified %zu and we have space for %zu\n", ctx->ret_size, *length);
      return CRYPTO_INVALID_SIZE;
   }
   *length = ctx->ret_size;
   *id     = ctx->curjob_swid;
   return CRYPTO_OK;
}
