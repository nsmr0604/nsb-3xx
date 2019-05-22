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


int spacc_clone_handle(spacc_device *spacc, int old_handle, void *cbdata)
{
   int new_handle;

   new_handle = spacc_job_request(spacc, spacc->job[old_handle].ctx_idx);
   if (new_handle < 0) {
      return new_handle;
   }
   spacc->job[new_handle]          = spacc->job[old_handle];
   spacc->job[new_handle].job_used = new_handle;
   spacc->job[new_handle].cbdata   = cbdata;
   return new_handle;
}

/*
 Allocates a job for spacc module context and initialize it with an
 appropriate type.
 */

int spacc_open (spacc_device *spacc, int enc, int hash, int ctxid, int secure_mode, spacc_callback cb, void *cbdata)
{
   int ret = CRYPTO_OK;
   int job_idx = 0;
   spacc_job *job = NULL;
   uint32_t ctrl = 0;

   if ((job_idx = spacc_job_request (spacc, ctxid)) < 0) {
      ELPHW_PRINT ("No jobs available: Try freeing up some job_idxs first)\n");
      ret = CRYPTO_FAILED;
   } else {
      job = &spacc->job[job_idx];

      if (secure_mode && job->ctx_idx > spacc->config.num_sec_ctx) {
         ELPHW_PRINT("Job context ID is outside allowed range for secure contexts\n");
         spacc_job_release (spacc, job_idx);
         return CRYPTO_FAILED;
      }

      job->auxinfo_cs_mode = 0;
      job->icv_len = 0;

      switch (enc) {
         case CRYPTO_MODE_RC4_40:
         case CRYPTO_MODE_RC4_128:
         case CRYPTO_MODE_RC4_KS:
            //if (ctx->rc4_key == NULL) {
            if (job->ctx_idx >= spacc->config.num_rc4_ctx) {
               spacc_job_release (spacc, job_idx);
               ELPHW_PRINT ("Out of RC4 contexts (try freeing up some job_idxs first)\n");
               return CRYPTO_FAILED;
            }
      }

      switch (enc) {
         case CRYPTO_MODE_NULL:
            break;
         case CRYPTO_MODE_RC4_40:
            ctrl |= CTRL_SET_CIPH_ALG (C_RC4);
            break;
         case CRYPTO_MODE_RC4_128:
            ctrl |= CTRL_SET_CIPH_ALG (C_RC4);
            break;
         case CRYPTO_MODE_RC4_KS:
            ctrl |= CTRL_SET_CIPH_ALG (C_RC4);
            break;
         case CRYPTO_MODE_AES_ECB:
            ctrl |= CTRL_SET_CIPH_ALG (C_AES);
            ctrl |= CTRL_SET_CIPH_MODE (CM_ECB);
            break;
         case CRYPTO_MODE_AES_CBC:
            ctrl |= CTRL_SET_CIPH_ALG (C_AES);
            ctrl |= CTRL_SET_CIPH_MODE (CM_CBC);
            break;
         case CRYPTO_MODE_AES_CS1:
            ctrl |= CTRL_SET_CIPH_ALG (C_AES);
            ctrl |= CTRL_SET_CIPH_MODE (CM_CBC);
            job->auxinfo_cs_mode = 1;
            break;
         case CRYPTO_MODE_AES_CS2:
            ctrl |= CTRL_SET_CIPH_ALG (C_AES);
            ctrl |= CTRL_SET_CIPH_MODE (CM_CBC);
            job->auxinfo_cs_mode = 2;
            break;
         case CRYPTO_MODE_AES_CS3:
            ctrl |= CTRL_SET_CIPH_ALG (C_AES);
            ctrl |= CTRL_SET_CIPH_MODE (CM_CBC);
            job->auxinfo_cs_mode = 3;
            break;
         case CRYPTO_MODE_AES_CFB:
            ctrl |= CTRL_SET_CIPH_ALG (C_AES);
            ctrl |= CTRL_SET_CIPH_MODE (CM_CFB);
            break;
         case CRYPTO_MODE_AES_OFB:
            ctrl |= CTRL_SET_CIPH_ALG (C_AES);
            ctrl |= CTRL_SET_CIPH_MODE (CM_OFB);
            break;
         case CRYPTO_MODE_AES_CTR:
            ctrl |= CTRL_SET_CIPH_ALG (C_AES);
            ctrl |= CTRL_SET_CIPH_MODE (CM_CTR);
            break;
         case CRYPTO_MODE_AES_CCM:
            ctrl |= CTRL_SET_CIPH_ALG (C_AES);
            ctrl |= CTRL_SET_CIPH_MODE (CM_CCM);
            break;
         case CRYPTO_MODE_AES_GCM:
            ctrl |= CTRL_SET_CIPH_ALG (C_AES);
            ctrl |= CTRL_SET_CIPH_MODE (CM_GCM);
            break;
         case CRYPTO_MODE_AES_F8:
            ctrl |= CTRL_SET_CIPH_ALG (C_AES);
            ctrl |= CTRL_SET_CIPH_MODE (CM_F8);
            break;
         case CRYPTO_MODE_AES_XTS:
            ctrl |= CTRL_SET_CIPH_ALG (C_AES);
            ctrl |= CTRL_SET_CIPH_MODE (CM_XTS);
            break;
         case CRYPTO_MODE_MULTI2_ECB:
            ctrl |= CTRL_SET_CIPH_ALG (C_MULTI2);
            ctrl |= CTRL_SET_CIPH_MODE (CM_ECB);
            break;
         case CRYPTO_MODE_MULTI2_CBC:
            ctrl |= CTRL_SET_CIPH_ALG (C_MULTI2);
            ctrl |= CTRL_SET_CIPH_MODE (CM_CBC);
            break;
         case CRYPTO_MODE_MULTI2_OFB:
            ctrl |= CTRL_SET_CIPH_ALG (C_MULTI2);
            ctrl |= CTRL_SET_CIPH_MODE (CM_OFB);
            break;
         case CRYPTO_MODE_MULTI2_CFB:
            ctrl |= CTRL_SET_CIPH_ALG (C_MULTI2);
            ctrl |= CTRL_SET_CIPH_MODE (CM_CFB);
            break;
         case CRYPTO_MODE_3DES_CBC:
         case CRYPTO_MODE_DES_CBC:
            ctrl |= CTRL_SET_CIPH_ALG (C_DES);
            ctrl |= CTRL_SET_CIPH_MODE (CM_CBC);
            break;
         case CRYPTO_MODE_3DES_ECB:
         case CRYPTO_MODE_DES_ECB:
            ctrl |= CTRL_SET_CIPH_ALG (C_DES);
            ctrl |= CTRL_SET_CIPH_MODE (CM_ECB);
            break;
         case CRYPTO_MODE_KASUMI_ECB:
            ctrl |= CTRL_SET_CIPH_ALG (C_KASUMI);
            ctrl |= CTRL_SET_CIPH_MODE (CM_ECB);
            break;
         case CRYPTO_MODE_KASUMI_F8:
            ctrl |= CTRL_SET_CIPH_ALG (C_KASUMI);
            ctrl |= CTRL_SET_CIPH_MODE (CM_F8);
            break;

         case CRYPTO_MODE_SNOW3G_UEA2:
            ctrl |= CTRL_SET_CIPH_ALG (C_SNOW3G_UEA2);
            ctrl |= CTRL_SET_CIPH_MODE (CM_ECB);
            break;

         case CRYPTO_MODE_ZUC_UEA3:
            ctrl |= CTRL_SET_CIPH_ALG (C_ZUC_UEA3);
            ctrl |= CTRL_SET_CIPH_MODE (CM_ECB);
            break;

         default:
            ret = CRYPTO_INVALID_ALG;
            break;
      }
      switch (hash) {
         case CRYPTO_MODE_NULL:
            ctrl |= CTRL_SET_HASH_ALG (H_NULL);
            break;
         case CRYPTO_MODE_HMAC_SHA1:
            ctrl |= CTRL_SET_HASH_ALG (H_SHA1);
            ctrl |= CTRL_SET_HASH_MODE (HM_HMAC);
            break;
         case CRYPTO_MODE_HMAC_MD5:
            ctrl |= CTRL_SET_HASH_ALG (H_MD5);
            ctrl |= CTRL_SET_HASH_MODE (HM_HMAC);
            break;
         case CRYPTO_MODE_HMAC_SHA224:
            ctrl |= CTRL_SET_HASH_ALG (H_SHA224);
            ctrl |= CTRL_SET_HASH_MODE (HM_HMAC);
            break;

         case CRYPTO_MODE_HMAC_SHA256:
            ctrl |= CTRL_SET_HASH_ALG (H_SHA256);
            ctrl |= CTRL_SET_HASH_MODE (HM_HMAC);
            break;

         case CRYPTO_MODE_HMAC_SHA384:
            ctrl |= CTRL_SET_HASH_ALG (H_SHA384);
            ctrl |= CTRL_SET_HASH_MODE (HM_HMAC);
            break;

         case CRYPTO_MODE_HMAC_SHA512:
            ctrl |= CTRL_SET_HASH_ALG (H_SHA512);
            ctrl |= CTRL_SET_HASH_MODE (HM_HMAC);
            break;

         case CRYPTO_MODE_HMAC_SHA512_224:
            ctrl |= CTRL_SET_HASH_ALG (H_SHA512_224);
            ctrl |= CTRL_SET_HASH_MODE (HM_HMAC);
            break;

         case CRYPTO_MODE_HMAC_SHA512_256:
            ctrl |= CTRL_SET_HASH_ALG (H_SHA512_256);
            ctrl |= CTRL_SET_HASH_MODE (HM_HMAC);
            break;

         case CRYPTO_MODE_SSLMAC_MD5:
            ctrl |= CTRL_SET_HASH_ALG (H_MD5);
            ctrl |= CTRL_SET_HASH_MODE (HM_SSLMAC);
            break;

         case CRYPTO_MODE_SSLMAC_SHA1:
            ctrl |= CTRL_SET_HASH_ALG (H_SHA1);
            ctrl |= CTRL_SET_HASH_MODE (HM_SSLMAC);
            break;


         case CRYPTO_MODE_HASH_SHA1:
            ctrl |= CTRL_SET_HASH_ALG (H_SHA1);
            ctrl |= CTRL_SET_HASH_MODE (HM_RAW);
            break;
         case CRYPTO_MODE_HASH_MD5:
            ctrl |= CTRL_SET_HASH_ALG (H_MD5);
            ctrl |= CTRL_SET_HASH_MODE (HM_RAW);
            break;

         case CRYPTO_MODE_HASH_SHA224:
            ctrl |= CTRL_SET_HASH_ALG (H_SHA224);
            ctrl |= CTRL_SET_HASH_MODE (HM_RAW);
            break;

         case CRYPTO_MODE_HASH_SHA256:
            ctrl |= CTRL_SET_HASH_ALG (H_SHA256);
            ctrl |= CTRL_SET_HASH_MODE (HM_RAW);
            break;

         case CRYPTO_MODE_HASH_SHA384:
            ctrl |= CTRL_SET_HASH_ALG (H_SHA384);
            ctrl |= CTRL_SET_HASH_MODE (HM_RAW);
            break;

         case CRYPTO_MODE_HASH_SHA512:
            ctrl |= CTRL_SET_HASH_ALG (H_SHA512);
            ctrl |= CTRL_SET_HASH_MODE (HM_RAW);
            break;

         case CRYPTO_MODE_HASH_SHA512_224:
            ctrl |= CTRL_SET_HASH_ALG (H_SHA512_224);
            ctrl |= CTRL_SET_HASH_MODE (HM_RAW);
            break;

         case CRYPTO_MODE_HASH_SHA512_256:
            ctrl |= CTRL_SET_HASH_ALG (H_SHA512_256);
            ctrl |= CTRL_SET_HASH_MODE (HM_RAW);
            break;

         case CRYPTO_MODE_MAC_XCBC:
            ctrl |= CTRL_SET_HASH_ALG (H_XCBC);
            ctrl |= CTRL_SET_HASH_MODE (HM_RAW);
            break;

         case CRYPTO_MODE_MAC_CMAC:
            ctrl |= CTRL_SET_HASH_ALG (H_CMAC);
            ctrl |= CTRL_SET_HASH_MODE (HM_RAW);
            break;

         case CRYPTO_MODE_MAC_KASUMI_F9:
            ctrl |= CTRL_SET_HASH_ALG (H_KF9);
            ctrl |= CTRL_SET_HASH_MODE (HM_RAW);
            break;

         case CRYPTO_MODE_MAC_SNOW3G_UIA2:
            ctrl |= CTRL_SET_HASH_ALG (H_SNOW3G_UIA2);
            ctrl |= CTRL_SET_HASH_MODE (HM_RAW);
            break;

         case CRYPTO_MODE_MAC_ZUC_UIA3:
            ctrl |= CTRL_SET_HASH_ALG (H_ZUC_UIA3);
            ctrl |= CTRL_SET_HASH_MODE (HM_RAW);
            break;

         case CRYPTO_MODE_HASH_CRC32:
            ctrl |= CTRL_SET_HASH_ALG (H_CRC32_I3E802_3);
            break;

         default:
            ret = CRYPTO_INVALID_ALG;
            break;
      }
   }

   ctrl |= (1UL << _SPACC_CTRL_MSG_BEGIN) | (1UL << _SPACC_CTRL_MSG_END);

   if (ret != CRYPTO_OK) {
      spacc_job_release (spacc, job_idx);
   } else {
      ret = job_idx;
      job->first_use   = 1;
      job->enc_mode    = enc;
      job->hash_mode   = hash;
      job->ckey_sz     = 0;
      job->hkey_sz     = 0;
      job->job_done    = 0;
      job->job_swid    = 0;
      job->job_secure  = ! !secure_mode;
      job->auxinfo_dir = 0;
      job->auxinfo_bit_align = 0;
      job->job_err     = CRYPTO_INPROGRESS;
      job->ctrl        = ctrl | CTRL_SET_CTX_IDX (job->ctx_idx);
      job->cb          = cb;
      job->cbdata      = cbdata;
   }
   return ret;
}
