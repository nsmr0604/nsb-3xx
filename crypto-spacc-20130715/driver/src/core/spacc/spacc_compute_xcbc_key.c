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

int spacc_compute_xcbc_key(spacc_device *spacc, const unsigned char *key, int keylen, unsigned char *xcbc_out)
{
         unsigned char *buf;
         dma_addr_t     bufphys;
         pdu_ddt        ddt;
         int err, i, handle, usecbc;
         unsigned char iv[16];

         // figure out if we can schedule the key ...
         if (spacc_isenabled(spacc, CRYPTO_MODE_AES_ECB, 16)) {
            usecbc = 0;
         } else if (spacc_isenabled(spacc, CRYPTO_MODE_AES_CBC, 16)) {
            usecbc = 1;
         } else {
            return -1;
         }

         memset(iv, 0, sizeof iv);
         memset(&ddt, 0, sizeof ddt);

         buf = pdu_dma_alloc(64, &bufphys);
         if (!buf) {
            return -EINVAL;
         }
         handle = -1;

         // set to 1111...., 2222...., 333...
         for (i = 0; i < 48; i++) {
            buf[i] = (i >> 4) + 1;
         }

         // build DDT ...
         err = pdu_ddt_init(&ddt, 1);
         if (err) {
            goto xcbc_err;
         }
         pdu_ddt_add(&ddt, bufphys, 48);

         // open a handle in either CBC or ECB mode
         handle = spacc_open(spacc, usecbc ? CRYPTO_MODE_AES_CBC : CRYPTO_MODE_AES_ECB, CRYPTO_MODE_NULL, -1, 0, NULL, NULL);
         if (handle < 0) {
            err = handle;
            goto xcbc_err;
         }
         spacc_set_operation(spacc, handle, OP_ENCRYPT, 0, 0, 0, 0, 0);

         if (usecbc) {
            // we can do the ECB work in CBC using three jobs with the IV reset to zero each time
            for (i = 0; i < 3; i++) {
               spacc_write_context(spacc, handle, SPACC_CRYPTO_OPERATION, key, keylen, iv, 16);
               err = spacc_packet_enqueue_ddt(spacc, handle, &ddt, &ddt, 16, (i*16)|((i*16)<<16), 0, 0, 0, 0);
               if (err != CRYPTO_OK) {
                  goto xcbc_err;
               }
               do {
                  err = spacc_packet_dequeue(spacc, handle);
               } while (err == CRYPTO_INPROGRESS);
               if (err != CRYPTO_OK) {
                  goto xcbc_err;
               }
            }
         } else {
            // do the 48 bytes as a single SPAcc job this is the ideal case but only possible
            // if ECB was enabled in the core
            spacc_write_context(spacc, handle, SPACC_CRYPTO_OPERATION, key, keylen, iv, 16);
            err = spacc_packet_enqueue_ddt(spacc, handle, &ddt, &ddt, 48, 0, 0, 0, 0, 0);
            if (err != CRYPTO_OK) {
               goto xcbc_err;
            }
            do {
               err = spacc_packet_dequeue(spacc, handle);
            } while (err == CRYPTO_INPROGRESS);
            if (err != CRYPTO_OK) {
               goto xcbc_err;
            }
         }

         // now we can copy the key
         memcpy(xcbc_out, buf, 48);
         memset(buf, 0, sizeof buf);
xcbc_err:
         pdu_dma_free(64, buf, bufphys);
         pdu_ddt_free(&ddt);
         if (handle >= 0) {
            spacc_close(spacc, handle);
         }
         if (err) {
            return -EINVAL;
         }
         return 0;
}
