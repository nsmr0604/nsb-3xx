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

// Function to unload data from an RC4 context.
int spacc_read_rc4_context (spacc_device * spacc, int job_idx, unsigned char * i, unsigned char * j, unsigned char * ctxdata)
{
   int ret = CRYPTO_OK;
   spacc_ctx *ctx = NULL;
   spacc_job *job = NULL;

   if (job_idx < 0 || job_idx > SPACC_MAX_JOBS) {
      return CRYPTO_INVALID_HANDLE;
   }

   job = &spacc->job[job_idx];
   ctx = context_lookup_by_job (spacc, job_idx);
   if (NULL == ctx) {
      ret = CRYPTO_FAILED;
   } else {
      unsigned char ijbuf[4];

      switch (job->enc_mode) {
         case CRYPTO_MODE_RC4_40:
         case CRYPTO_MODE_RC4_128:
         case CRYPTO_MODE_RC4_KS: break;
         default: return CRYPTO_FAILED;
      }

      pdu_from_dev32_s (ctxdata, ctx->rc4_key, SPACC_CTX_RC4_IJ >> 2, spacc_endian);
      pdu_from_dev32_s (ijbuf, ctx->rc4_key + (SPACC_CTX_RC4_IJ >> 2), 1, spacc_endian);

      *i = ijbuf[0];
      *j = ijbuf[1];
   }
   return ret;
}
