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

/* Context manager */

/* This will reset all reference counts, pointers, etc */
void spacc_ctx_init_all (spacc_device *spacc)
{
   int x;
   spacc_ctx * ctx;
   unsigned long lock_flag;

   PDU_LOCK(&spacc->ctx_lock, lock_flag);
   /* initialize contexts */
   for (x = 0; x < spacc->config.num_ctx; x++) {
      ctx = &spacc->ctx[x];

      /* sets everything including ref_cnt and ncontig to 0 */
      memset (ctx, 0, sizeof (*ctx));

      ctx->ciph_key = spacc->regmap + SPACC_CTX_CIPH_KEY + (x * spacc->config.ciph_page_size);
      ctx->hash_key = spacc->regmap + SPACC_CTX_HASH_KEY + (x * spacc->config.hash_page_size);
      if (x < spacc->config.num_rc4_ctx) {
         ctx->rc4_key = spacc->regmap + SPACC_CTX_RC4_CTX + (x * 512);
      }
   }
   PDU_UNLOCK(&spacc->ctx_lock, lock_flag);
}

/* ctx_id is requested */
/* This function is not meant to be called directly, it should be called from the job manager */
int spacc_ctx_request(spacc_device *spacc, int ctx_id, int ncontig)
{
   int ret;
   int x, y, count;
   unsigned long lock_flag, timeout;

   if (spacc == NULL) {
      ELPHW_PRINT ("spacc_ctx_request::spacc cannot be NULL\n");
      return -1;
   }
   if (ctx_id > spacc->config.num_ctx) {
      ELPHW_PRINT ("spacc_ctx_request::ctx_id cannot be larger than availble\n");
      return -1;
   }
   if (ncontig < 1 || ncontig > spacc->config.num_ctx) {
      ELPHW_PRINT ("spacc_ctx_request::Invalid number of contiguous contexts requested\n");
      return -1;
   }
   /* test if spacc_hsm_shared, then max is 1 */
   if ((ncontig > 1) && spacc->config.is_hsm_shared) {
      ELPHW_PRINT ("spacc_ctx_request::HSM shared mode can only have 1 context requested\n");
      return -1;
   }

   ret = CRYPTO_OK;

   PDU_LOCK(&spacc->ctx_lock, lock_flag);
   /* allocating scheme, look for contiguous contexts. Free contexts have a ref_cnt of 0.  */

   if (spacc->config.is_hsm_shared) {
      /* request HSM shared CTX */
      if (ctx_id == -1) {
         /* request next available */
         ctx_id = 0;
      } else {
         /* request a specific ctx */
         ctx_id |= SPACC_CTX_CMD_IDX_REQ;
      }
//rintk("Port: %d is attempting to allocate...\n", spacc->is_secure_port);
      pdu_io_write32 (spacc->regmap + SPACC_REG_HSM_CTX_CMD, ctx_id | SPACC_CTX_CMD_REQ);

      /* wait for ready flag */
      timeout = 100000UL;
      while (--timeout && (((ctx_id = pdu_io_read32(spacc->regmap + SPACC_REG_HSM_CTX_STAT)) & SPACC_CTX_STAT_RDY) != SPACC_CTX_STAT_RDY)){};
      if (!timeout) {
         ELPHW_PRINT("spacc_ctx_request::Timeout requesting handle from HSM CTX manager\n");
         ret = CRYPTO_FAILED;
      } else {
         if (ctx_id & SPACC_CTX_STAT_SUCCESS) {
            ctx_id &= spacc->config.ctx_mask;
//printk("Port: %d got CTX %u\n", spacc->is_secure_port, ctx_id);
         } else {
            ELPHW_PRINT("spacc_ctx_request::HSM CTX manager returned an error allocating a handle\n");
            ret = CRYPTO_FAILED;
         }
      }
   } else {

      /* if specific ctx_id is requested, test the ncontig and then bump the ref_cnt */
      if (ctx_id != -1) {
         if ((&spacc->ctx[ctx_id])->ncontig != ncontig - 1) {
            ELPHW_PRINT ("spacc_ctx_request::ncontig mismatch [%d] requested but [%d] already set\n", ncontig, (&spacc->ctx[ctx_id])->ncontig +1);
            ret = -1;
         }

      } else {

         /* check to see if ncontig are free */

         /* loop over all available contexts to find the first ncontig empty ones */
         for (x = 0; x <= (spacc->config.num_ctx - ncontig); ) {
            count = ncontig;
            while (count) {
               if ((&spacc->ctx[x + count -1])->ref_cnt != 0) {
                  /* incr x to past failed count location */
                  x = x + count;
                  break;
               } else {
                  count--;
               }
            }
            if (count != 0) {
               ret = -1;
               /* test next x */
            } else {
               ctx_id = x;
               ret = CRYPTO_OK;
               break;
            }
         }
      }
   }

   if (ret == CRYPTO_OK) {
      /* ctx_id is good so mark used */
      for (y = 0; y < ncontig; y++) {
         (&spacc->ctx[ctx_id + y])->ref_cnt++;
      }
      (&spacc->ctx[ctx_id])->ncontig = ncontig - 1;
   } else {
      ctx_id = -1;
   }
   //ELPHW_PRINT ("ctx_request:: ctx_id= [%d] ncontig[%d]\n", ctx_id, ncontig);

   PDU_UNLOCK(&spacc->ctx_lock, lock_flag);
   return ctx_id;
}

int spacc_ctx_release(spacc_device *spacc, int ctx_id)
{
   int ret = CRYPTO_OK;
   unsigned long lock_flag, timeout;
   int ncontig;
   int y;

   if (ctx_id < 0 || ctx_id > spacc->config.num_ctx) {
      return -1;
   }

   PDU_LOCK(&spacc->ctx_lock, lock_flag);
   /* release the base context and contiguous block */
   ncontig = (&spacc->ctx[ctx_id])->ncontig;
   for (y = 0; y <= ncontig; y++) {
      if ((&spacc->ctx[ctx_id + y])->ref_cnt > 0) {
         (&spacc->ctx[ctx_id + y])->ref_cnt--;
      }
      //ELPHW_PRINT ("spacc_ctx_release:: ctx:[%d] refcnt[%d]\n", ctx_id, (&spacc->ctx[ctx_id + y])->ref_cnt);
   }

   if ((&spacc->ctx[ctx_id])->ref_cnt == 0) {
      (&spacc->ctx[ctx_id])->ncontig = 0;
   }

   if (spacc->config.is_hsm_shared) {
//printk("Port: %d freed CTX %u\n", spacc->is_secure_port, ctx_id);
      /* release HSM shared CTX */
      pdu_io_write32 (spacc->regmap + SPACC_REG_HSM_CTX_CMD, ctx_id);
      /* wait for ready flag */
      timeout = 100000UL;
      while (--timeout && ((pdu_io_read32(spacc->regmap + SPACC_REG_HSM_CTX_STAT) & SPACC_CTX_STAT_RDY) != SPACC_CTX_STAT_RDY)){};
      if (!timeout) {
         ELPHW_PRINT("Failed to release context to HSM\n");
         return CRYPTO_FAILED;
      }
   }

   PDU_UNLOCK(&spacc->ctx_lock, lock_flag);
   return ret;
}

