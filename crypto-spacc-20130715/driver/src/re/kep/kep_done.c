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
#include "elpkep.h"

/* the basic idea here is this function "finishes" a job, but not necessarily the job
the caller was interested in.  So we read the STAT fifo search the ctx table for the job
id and mark the job_done flag.  This function returns CRYPTO_OK if the job_done flag
for the callers handle is marked true.  Otherwise it returns INPROGRESS */

int kep_done (kep_device * kep, int handle)
{
   unsigned long lock_flag;
   int ret;

   if (handle != -1 && kep_is_valid (kep, handle)) {
      ELPHW_PRINT ("kep_done::Invalid handle specified\n");
      return CRYPTO_FAILED;
   }
   if (handle != -1 && kep->ctx[handle].job_done == 1) {
      return CRYPTO_OK;
   }

   if (handle != -1 && kep->ctx[handle].job_id == -1) {
      ELPHW_PRINT ("kep_done::Invalid handle as job is marked as not active\n");
      return CRYPTO_FAILED;
   }

   PDU_LOCK(&kep->lock, lock_flag);

   // is job already done?
   if (handle != -1 && kep->ctx[handle].job_done == 1) {
      ret = CRYPTO_OK;
      goto ERR;
   }

   if (!KEP_FIFO_STAT_EMPTY (pdu_io_read32 (kep->regmap + KEP_REG_FIFO_STAT))) {
      uint32_t status, swid, retcode, x, y;

      pdu_io_write32 (kep->regmap + KEP_REG_POP, 1);

      status = pdu_io_read32 (kep->regmap + KEP_REG_STATUS);
      swid = KEP_STATUS_SWID (status);
      retcode = KEP_STATUS_RETCODE (status);

      switch (retcode) {
         case 0:               // OK
            y = kep->spacc->config.num_ctx;
            for (x = 0; x < y; x++) {
               if (swid == kep->ctx[x].job_id) {
                  kep->ctx[x].job_done = 1;
                  if (kep->ctx[x].cb) {
                     kep->ctx[x].cb(kep, kep->ctx[x].cbdata);
                  }
                  kep->ctx[x].job_id = -1;
                  goto end;
               }
            }
            ELPHW_PRINT ("kep_done::Job ID not found!\n");
            ret = CRYPTO_FAILED;
            goto ERR;
         default:
            ELPHW_PRINT ("kep_done::KEP returned error %d\n", retcode);
            ret = CRYPTO_FAILED;
            goto ERR;
      }
   } else {
     ELPHW_PRINT("KEP: done called but no job posted\n");
   }
 end:
   if (handle != -1) {
      if (kep->ctx[handle].job_done == 1) {
         ret = CRYPTO_OK;
      } else {
         ret = CRYPTO_INPROGRESS;
      }
   } else {
      ret = CRYPTO_OK;
   }
ERR:
   PDU_UNLOCK(&kep->lock, lock_flag);
   return ret;
}
