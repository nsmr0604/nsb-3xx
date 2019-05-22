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

static void put_buf(unsigned char *dst, const unsigned char *src, int off, int n, int len)
{
	if (!src) return;
	while (n && (off < len)) {
		dst[off++] = *src++;
		--n;
	}
}

/* Write appropriate context data which depends on operation and mode */
int spacc_write_context (spacc_device * spacc, int job_idx, int op, const unsigned char * key, int ksz, const unsigned char * iv, int ivsz)
{
   int ret = CRYPTO_OK;
   spacc_ctx *ctx = NULL;
   spacc_job *job = NULL;
   
   unsigned char buf[300];
   int buflen;

   if (job_idx < 0 || job_idx > SPACC_MAX_JOBS) {
      return CRYPTO_INVALID_HANDLE;
   }

   job = &spacc->job[job_idx];
   ctx = context_lookup_by_job(spacc, job_idx);
  
   if ((NULL == job) || (NULL == ctx)) {
      ret = CRYPTO_FAILED;
   } else {
      switch (op) {
         case SPACC_CRYPTO_OPERATION:
            // get page size and then read so we can do a read-modify-write cycle
            buflen = min(sizeof(buf),(unsigned)spacc->config.ciph_page_size);
            pdu_from_dev32_s(buf,  ctx->ciph_key, buflen>>2, spacc_endian);
            switch (job->enc_mode) {
               case CRYPTO_MODE_RC4_40:
                  put_buf(buf, key, 0, 8, buflen);
                  break;
               case CRYPTO_MODE_RC4_128:
                  put_buf(buf, key, 0, 16, buflen);
                  break;
               case CRYPTO_MODE_RC4_KS:
                  if (key) {
                     ret = spacc_write_rc4_context (spacc, job_idx, key[0], key[1], key + 2);
                  }
                  break;
               case CRYPTO_MODE_AES_ECB:
               case CRYPTO_MODE_AES_CBC:
               case CRYPTO_MODE_AES_CS1:
               case CRYPTO_MODE_AES_CS2:
               case CRYPTO_MODE_AES_CS3:
               case CRYPTO_MODE_AES_CFB:
               case CRYPTO_MODE_AES_OFB:
               case CRYPTO_MODE_AES_CTR:
               case CRYPTO_MODE_AES_CCM:
               case CRYPTO_MODE_AES_GCM:
                  put_buf(buf, key, 0, ksz, buflen);
                  if (iv) {
                     unsigned char one[4] = { 0, 0, 0, 1 };
                     put_buf(buf, iv, 32, ivsz, buflen);
                     if (ivsz == 12 && job->enc_mode == CRYPTO_MODE_AES_GCM) {
                        put_buf(buf, one, 11*4, 4, buflen);
                     }
                  }
                  break;
               case CRYPTO_MODE_AES_F8:
                  if (key) {
                     put_buf(buf, key + ksz, 0,  ksz, buflen);
                     put_buf(buf, key,       48, ksz, buflen);
                  }
                  put_buf(buf, iv,        32,  16, buflen);
                  break;
               case CRYPTO_MODE_AES_XTS:
                  if (key) {
                     put_buf(buf, key,           0, ksz>>1, buflen);
                     put_buf(buf, key+(ksz>>1), 48, ksz>>1, buflen);
                     ksz = ksz >> 1;   // divide by two since that's what we program the hardware with
                  }
                  put_buf(buf, iv, 32, 16, buflen);
                  break;
#if 0                  
               case CRYPTO_MODE_MULTI2_ECB:
               case CRYPTO_MODE_MULTI2_CBC:
               case CRYPTO_MODE_MULTI2_OFB:
               case CRYPTO_MODE_MULTI2_CFB:
                  // Number of rounds at the end of the IV
                  if (key) {
                     pdu_to_dev32_s (ctx->ciph_key, key, ksz >> 2, spacc_endian);
                  }
                  if (iv) {
                     pdu_to_dev32_s (&ctx->ciph_key[10], iv, ivsz >> 2, spacc_endian);
                  }
                  if (ivsz == 0) {
                     ctx->ciph_key[0x30 / 4] = 0x80000000;  // default to 128 rounds
                  }
                  break;
#endif

               case CRYPTO_MODE_3DES_CBC:
               case CRYPTO_MODE_3DES_ECB:
               case CRYPTO_MODE_DES_CBC:
               case CRYPTO_MODE_DES_ECB:
                  put_buf(buf, iv, 0, 8, buflen);
 				  put_buf(buf, key, 8, ksz, buflen);
                  break;

               case CRYPTO_MODE_KASUMI_ECB:
               case CRYPTO_MODE_KASUMI_F8:
                  put_buf(buf, iv, 16, 8, buflen);
                  put_buf(buf, key, 0, 16, buflen);
                  break;

               case CRYPTO_MODE_SNOW3G_UEA2:
               case CRYPTO_MODE_ZUC_UEA3:
                  put_buf(buf, key, 0, 32, buflen);
                  break;

               case CRYPTO_MODE_NULL:
               default:
                  break;
            }
            if (key) {
               job->ckey_sz = SPACC_SET_CIPHER_KEY_SZ (ksz);
               job->first_use = 1;
            }
            pdu_to_dev32_s (ctx->ciph_key, buf, buflen >> 2, spacc_endian);
            break;
         case SPACC_HASH_OPERATION:
            // get page size and then read so we can do a read-modify-write cycle
            buflen = min(sizeof(buf),(unsigned)spacc->config.hash_page_size);
            pdu_from_dev32_s(buf,  ctx->hash_key, buflen>>2, spacc_endian);
            switch (job->hash_mode) {
               case CRYPTO_MODE_MAC_XCBC:
                  if (key) {
                     put_buf(buf, key + (ksz - 32), 32,         32, buflen);
                     put_buf(buf, key,               0, (ksz - 32), buflen);
                     job->hkey_sz = SPACC_SET_HASH_KEY_SZ (ksz - 32);
                  }
                  break;
               case CRYPTO_MODE_HASH_CRC32:
                  put_buf(buf, iv, 0, ivsz, buflen);
                  job->hkey_sz = SPACC_SET_HASH_KEY_SZ (ivsz);
                  break;
               case CRYPTO_MODE_MAC_SNOW3G_UIA2:
               case CRYPTO_MODE_MAC_ZUC_UIA3:
                  if (key) {
                     put_buf(buf, key, 0, ksz, buflen);
                     job->hkey_sz = SPACC_SET_HASH_KEY_SZ (ksz);
                  }
                  break;
               default:
                  if (key) {
                     job->hkey_sz = SPACC_SET_HASH_KEY_SZ (ksz);
                     put_buf(buf, key, 0, ksz, buflen);
                  }
            }
            pdu_to_dev32_s (ctx->hash_key, buf, buflen >> 2, spacc_endian);
            break;
         default:
            ret = CRYPTO_INVALID_MODE;
            break;
      }
   }
   return ret;
}
