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

// prevent reading passed the end of the buffer
static void read_from(unsigned char *dst, unsigned char *src, int off, int n, int max)
{
   if (!dst) return;
   while (off < max && n) {
      *dst++ = src[off++];
      --n;
   }
}

int spacc_read_context (spacc_device * spacc, int job_idx, int op, unsigned char * key, int ksz, unsigned char * iv, int ivsz)
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

   if (NULL == ctx) {
      ret = CRYPTO_FAILED;
   } else {
       switch (op) {
         case SPACC_CRYPTO_OPERATION:
            buflen = min(sizeof(buf),(unsigned)spacc->config.ciph_page_size);
            pdu_from_dev32_s(buf,  ctx->ciph_key, buflen>>2, spacc_endian);
            switch (job->enc_mode) {
               case CRYPTO_MODE_RC4_40:
                  read_from(key, buf, 0, 8, buflen);
                  break;
               case CRYPTO_MODE_RC4_128:
                  read_from(key, buf, 0, 16, buflen);
                  break;
               case CRYPTO_MODE_RC4_KS:
                  if (key && ksz == 258) {
                     ret = spacc_read_rc4_context (spacc, job_idx, &key[0], &key[1], key + 2);
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
                  read_from(key, buf, 0, ksz, buflen);
                  read_from(iv, buf,  32, 16, buflen);
                  break;
               case CRYPTO_MODE_AES_F8:
                  if (key) {
                     read_from(key+ksz, buf, 0,  ksz, buflen);
                     read_from(key,     buf, 48, ksz, buflen);
                  }
                  read_from(iv, buf, 32, 16, buflen);
                  break;
               case CRYPTO_MODE_AES_XTS:
                  if (key) {
                     read_from(key,            buf,  0, ksz>>1, buflen);
                     read_from(key + (ksz>>1), buf, 48, ksz>>1, buflen);
                  }
                  read_from(iv, buf, 32, 16, buflen);
                  break;
#if 0
               case CRYPTO_MODE_MULTI2_ECB:
               case CRYPTO_MODE_MULTI2_CBC:
               case CRYPTO_MODE_MULTI2_OFB:
               case CRYPTO_MODE_MULTI2_CFB:
                  // Number of rounds at the end of the IV
                  if (key) {
                     pdu_from_dev32_s (key, ctx->ciph_key, ksz >> 2, spacc_endian);
                  }
                  if (iv) {
                     pdu_from_dev32_s (iv, &ctx->ciph_key[10], ivsz >> 2, spacc_endian);
                  }
                  break;
#endif
               case CRYPTO_MODE_3DES_CBC:
               case CRYPTO_MODE_3DES_ECB:
                  read_from(iv,  buf, 0,  8, buflen);
                  read_from(key, buf, 8, 24, buflen);
                  break;
               case CRYPTO_MODE_DES_CBC:
               case CRYPTO_MODE_DES_ECB:
                  read_from(iv,  buf, 0, 8, buflen);
                  read_from(key, buf, 8, 8, buflen);
                  break;

               case CRYPTO_MODE_KASUMI_ECB:
               case CRYPTO_MODE_KASUMI_F8:
                  read_from(iv,  buf, 16,  8, buflen);
                  read_from(key, buf, 0,  16, buflen);
                  break;

               case CRYPTO_MODE_SNOW3G_UEA2:
               case CRYPTO_MODE_ZUC_UEA3:
                  read_from(key, buf, 0, 32, buflen);
                  break;

               case CRYPTO_MODE_NULL:
               default:
                  break;
            }

            break;
         case SPACC_HASH_OPERATION:
            buflen = min(sizeof(buf),(unsigned)spacc->config.hash_page_size);
            pdu_from_dev32_s(buf, ctx->hash_key, buflen>>2, spacc_endian);
            switch (job->hash_mode) {
               case CRYPTO_MODE_MAC_XCBC:
                  if (key && ksz <= 64) {
                     read_from(key + (ksz - 32), buf, 32, 32,       buflen);
                     read_from(key,              buf, 0,  ksz - 32, buflen);
                  }
                  break;
               case CRYPTO_MODE_HASH_CRC32:
                  read_from(iv, buf, 0, ivsz, buflen);
                  break;
               case CRYPTO_MODE_MAC_SNOW3G_UIA2:
               case CRYPTO_MODE_MAC_ZUC_UIA3:
                  read_from(key, buf, 0,  32, buflen);
                  break;
               default:
                  read_from(key, buf, 0, ksz, buflen);
            }
            break;
         default:
            ret = CRYPTO_INVALID_MODE;
            break;
      }
   }
   return ret;
}
