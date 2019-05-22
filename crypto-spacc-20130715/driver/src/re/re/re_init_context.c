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


// init an RE context with specific SPAcc context
int re_init_context_ex(re_device * re, int handle, int ctxid)
{
   //Make sure the the context is vaid, and if it is create it
   elp_re_ctx *ctx;
   if (handle < 0 || handle >= re->config.num_ctx) {
      return CRYPTO_INVALID_CONTEXT;
   }

   ctx = &re->re_contexts[handle];

   ctx->sa    = re->sa_pool_virt + RE_SA_SIZE * ctxid;
   ctx->samap = re->sa_pool_phys + RE_SA_SIZE * ctxid;

   ctx->jobdone      = 0;
   ctx->curjob_swid  = -1;
   ctx->ret_stat     = 0;
   ctx->ret_prot     = 0;
   ctx->ret_size     = 0;
   ctx->spacc_handle = ctxid;

   return CRYPTO_OK;
}

/*
 * Initialize a contex, by setting up the SA, DDT's and all physical and virtual pointers
 */
int re_init_context (re_device * re, int handle)
{
   return re_init_context_ex(re, handle, handle);
}
