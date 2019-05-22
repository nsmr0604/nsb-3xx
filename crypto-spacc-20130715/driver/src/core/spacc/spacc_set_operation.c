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

int spacc_set_operation (spacc_device * spacc, int handle, int op, uint32_t prot, uint32_t icvcmd, uint32_t icvoff, uint32_t icvsz, uint32_t sec_key)
{
   int ret = CRYPTO_OK;
   spacc_job *job = NULL;

   if (handle < 0 || handle > SPACC_MAX_JOBS) {
      return CRYPTO_INVALID_HANDLE;
   }

   //job = job_lookup (spacc, handle);
   job = &spacc->job[handle];
   if (NULL == job) {
      ret = CRYPTO_FAILED;
   } else {
      if (op == OP_ENCRYPT) {
         job->op = OP_ENCRYPT;
         job->ctrl |= CTRL_SET_ENCRYPT;
      } else {
         job->op = OP_DECRYPT;
         job->ctrl &= ~CTRL_SET_ENCRYPT;
      }
      switch (prot) {
         case ICV_HASH:        /* HASH of plaintext */
            job->ctrl |= CTRL_SET_ICV_PT;
            break;
         case ICV_HASH_ENCRYPT: /* HASH the plaintext and encrypt the lot */
            job->ctrl |= CTRL_SET_ICV_ENC;   /* ICV_PT and ICV_APPEND must be set too */
            job->ctrl |= CTRL_SET_ICV_PT;
            job->ctrl |= CTRL_SET_ICV_APPEND;   /* This mode is not valid when BIT_ALIGN != 0 */
            break;
         case ICV_ENCRYPT_HASH: /* HASH the ciphertext */
            job->ctrl &= ~(CTRL_SET_ICV_PT);    // ICV_PT=0
            job->ctrl &= ~(CTRL_SET_ICV_ENC);   // ICV_ENC=0
            break;
         case ICV_IGNORE:
            break;
         default:
            ret = CRYPTO_INVALID_MODE;
            break;
      }

      job->icv_len = icvsz;

      switch (icvcmd) {
         case IP_ICV_OFFSET:
            job->icv_offset = icvoff;
            job->ctrl &= ~CTRL_SET_ICV_APPEND;
            break;
         case IP_ICV_APPEND:
            job->ctrl |= CTRL_SET_ICV_APPEND;
            break;
         case IP_ICV_IGNORE:
            break;
         default:
            ret = CRYPTO_INVALID_MODE;
            break;
      }

      if (sec_key) {
         job->ctrl |= CTRL_SET_SEC_KEY;
      }
   }
   return ret;
}
