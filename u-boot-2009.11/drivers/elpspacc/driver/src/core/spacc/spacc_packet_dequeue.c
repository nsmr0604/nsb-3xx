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

static inline uint32_t _spacc_get_stat_cnt (spacc_device * spacc)
{
   uint32_t fifo;

   if (spacc->config.is_qos) {
      fifo = SPACC_FIFO_STAT_STAT_CNT_GET_QOS (pdu_io_read32 (spacc->regmap + SPACC_REG_FIFO_STAT));
   } else {
      fifo = SPACC_FIFO_STAT_STAT_CNT_GET (pdu_io_read32 (spacc->regmap + SPACC_REG_FIFO_STAT));
   }
   return fifo;
}

int spacc_pop_packets_ex (spacc_device * spacc, int *num_popped, unsigned long *lock_flag)
{
   int ret = CRYPTO_INPROGRESS;
  // spacc_ctx *ctx = NULL;
   spacc_job *job = NULL;
   uint32_t cmdstat, swid;
   int jobs;

   *num_popped = 0;

   while ((jobs = _spacc_get_stat_cnt(spacc))) {
      while (jobs-- > 0) {
         /* write the pop register to get the next job */
         pdu_io_write32 (spacc->regmap + SPACC_REG_STAT_POP, 1);
         cmdstat = pdu_io_read32 (spacc->regmap + SPACC_REG_STATUS);

         swid = SPACC_STATUS_SW_ID_GET(cmdstat);

         if (spacc->job_lookup[swid] == SPACC_JOB_IDX_UNUSED) {
            ELPHW_PRINT ("Invalid sw id (%d) popped off the stack", swid);
            ret = CRYPTO_FAILED;
            goto ERR;
         }

         /* find the associated job with popped swid */
         job = job_lookup_by_swid (spacc, swid);
         if (NULL == job) {
            ret = CRYPTO_FAILED;
            ELPHW_PRINT ("Failed to find job for ID %d\n", swid);
            goto ERR;
         }

         /* mark job as done */
         job->job_done = 1;
         spacc->job_lookup[swid] = SPACC_JOB_IDX_UNUSED;
         switch (SPACC_GET_STATUS_RET_CODE (cmdstat)) {
            case SPACC_ICVFAIL:
               ret = CRYPTO_AUTHENTICATION_FAILED;
               break;
            case SPACC_MEMERR:
               ret = CRYPTO_MEMORY_ERROR;
               break;
            case SPACC_BLOCKERR:
               ret = CRYPTO_INVALID_BLOCK_ALIGNMENT;
               break;
            case SPACC_SECERR:
               ret = CRYPTO_FAILED;
               break;

            case SPACC_OK:
   #ifdef SECURE_MODE
               if (job->job_secure && !(cmdstat & (1 << _SPACC_STATUS_SEC_CMD))) {
                  ret = CRYPTO_INPROGRESS;
                  break;
               }
   #endif
               ret = CRYPTO_OK;
               break;
         }

         job->job_err = ret;

         /*
          * We're done touching the SPAcc hw, so release the lock across the
          * job callback.  It must be reacquired before continuing to the next
          * iteration.
          */

         if (job->cb) {
            PDU_UNLOCK(&spacc->lock, *lock_flag);
            job->cb(spacc, job->cbdata);
            PDU_LOCK(&spacc->lock, *lock_flag);
         }

         (*num_popped)++;

      }
   }
   //if (!*num_popped) { ELPHW_PRINT("ERROR: Failed to pop a single job\n"); }
ERR:
   // are there jobs in the buffer?
   if (spacc->job_buffer_use) {
      int x, y;
      spacc->job_buffer_use = 0;
      for (x = 0; x < SPACC_MAX_JOB_BUFFERS; x++) {
         if (spacc->job_buffer[x].active) {
            y = spacc_packet_enqueue_ddt_ex(spacc, 0,
                  spacc->job_buffer[x].job_idx,
                  spacc->job_buffer[x].src,
                  spacc->job_buffer[x].dst,
                  spacc->job_buffer[x].proc_sz,
                  spacc->job_buffer[x].aad_offset,
                  spacc->job_buffer[x].pre_aad_sz,
                  spacc->job_buffer[x].post_aad_sz,
                  spacc->job_buffer[x].iv_offset,
                  spacc->job_buffer[x].prio);
            if (y != CRYPTO_FIFO_FULL) {
               spacc->job_buffer[x].active = 0;
            } else {
               spacc->job_buffer_use = 1;
               break;
            }
         }
      }
   }

   if (spacc->op_mode == SPACC_OP_MODE_WD) {
      spacc_set_wd_count(spacc, spacc->config.wd_timer); // reset the WD timer to the original value
   }

   if (*num_popped && spacc->spacc_notify_jobs != NULL) {
      spacc->spacc_notify_jobs(spacc);
   }

   return ret;
}

int spacc_pop_packets (spacc_device * spacc, int *num_popped)
{
   unsigned long lock_flag;
   int err;
   PDU_LOCK(&spacc->lock, lock_flag);
   err = spacc_pop_packets_ex(spacc, num_popped, &lock_flag);
   PDU_UNLOCK(&spacc->lock, lock_flag);
   return err;
}


/* test if done */
int spacc_packet_dequeue (spacc_device * spacc, int job_idx)
{
   int ret = CRYPTO_OK;
   spacc_job *job = &spacc->job[job_idx];
   unsigned long lock_flag;

   PDU_LOCK(&spacc->lock, lock_flag);

   if (job == NULL && !(job_idx == SPACC_JOB_IDX_UNUSED)) {
      ret = CRYPTO_FAILED;
   } else {
      if (job->job_done) {
         job->job_done  = 0;
         ret = job->job_err;
      } else {
         ret = CRYPTO_INPROGRESS;
      }
   }

   PDU_UNLOCK(&spacc->lock, lock_flag);
   return ret;
}
