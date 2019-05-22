
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

/* Job manager */

/* This will reset all job data, pointers, etc */
void spacc_job_init_all (spacc_device *spacc)
{
   int x;
   spacc_job * job;

   for (x = 0; x < (SPACC_MAX_JOBS); x++) {
      job = &spacc->job[x];
      memset (job, 0, sizeof (spacc_job));
      job->job_swid = SPACC_JOB_IDX_UNUSED;
      job->job_used = SPACC_JOB_IDX_UNUSED;
      spacc->job_lookup[x] = SPACC_JOB_IDX_UNUSED;
   }

   /* if this is a secure mode HSM we have to release all of the contexts first (only issue this for the zeroth) */
   if (spacc->config.is_hsm_shared && spacc->config.is_secure_port == 1) {
      unsigned long x, timeout;
      for (x = 0; x < spacc->config.num_ctx; x++) {
         pdu_io_write32 (spacc->regmap + SPACC_REG_HSM_CTX_CMD, x);

         /* wait for ready flag */
         timeout = 1000000UL;
         while (--timeout && ((pdu_io_read32(spacc->regmap + SPACC_REG_HSM_CTX_STAT) & SPACC_CTX_STAT_RDY) != SPACC_CTX_STAT_RDY)){};
         if (!timeout) {
            ELPHW_PRINT("WARNING:  Failed to release context %lu upon init\n", x);
         }
      }
   }


}

/* get a new job id and use a specific ctx_idx or -1 for a new one */
int spacc_job_request (spacc_device * spacc, int ctx_idx)
{
   int x, ret;
   unsigned long lock_flag;
   spacc_job *job;

   if (spacc == NULL) {
      ELPHW_PRINT ("spacc_job_request::spacc cannot be NULL\n");
      return -1;
   }

   PDU_LOCK(&spacc->lock, lock_flag);
   /* find the first availble job id */
   for (x = 0; x < SPACC_MAX_JOBS; x++) {
      job = &spacc->job[x];
      if (job->job_used == SPACC_JOB_IDX_UNUSED) {
         job->job_used = x;
         break;
      }
   }

   if (x == SPACC_MAX_JOBS) {
      ELPHW_PRINT ("spacc_job_request::max number of jobs reached\n");
      ret = -1;
   } else {
      /* associate a single context to go with job */
      ret = spacc_ctx_request(spacc, ctx_idx, 1);
      if (ret != -1) {
         job->ctx_idx = ret;
         ret = x;
      }
      //ELPHW_PRINT ("spacc_job_request::ctx request [%d]\n", ret);
   }

   PDU_UNLOCK(&spacc->lock, lock_flag);
   return ret;
}

int spacc_job_release (spacc_device * spacc, int job_idx)
{
   int ret;
   unsigned long lock_flag;
   spacc_job *job;

   if (spacc == NULL) {
      ELPHW_PRINT ("spacc_job_release::spacc cannot be NULL\n");
      return -1;
   }
   if (job_idx >= SPACC_MAX_JOBS) {
      ELPHW_PRINT ("spacc_job_release::job_idx > SPACC_MAX_JOBS\n");
      return -1;
   }

   PDU_LOCK(&spacc->lock, lock_flag);

   job = &spacc->job[job_idx];
   /* release context that goes with job */
   ret = spacc_ctx_release(spacc, job->ctx_idx);
   job->ctx_idx  = SPACC_CTX_IDX_UNUSED;
   job->job_used = SPACC_JOB_IDX_UNUSED;
   job->cb       = NULL; // disable any callback

   /* NOTE: this leaves ctrl data in memory */

   PDU_UNLOCK(&spacc->lock, lock_flag);
   return ret;
}

/* Return a context structure for a job idx or null if invalid */
spacc_ctx * context_lookup_by_job (spacc_device * spacc, int job_idx)
{
   if ((job_idx < 0) || (job_idx >= SPACC_MAX_JOBS)) {
      ELPHW_PRINT ("context_lookup::Invalid job number\n");
      return NULL;
   }
   return &spacc->ctx[(&spacc->job[job_idx])->ctx_idx];
}

/* Return a job structure for a swid or null if invalid */
spacc_job * job_lookup_by_swid (spacc_device * spacc, int swid)
{
   if ((swid < 0) || (swid >= SPACC_MAX_JOBS)) {
      ELPHW_PRINT ("job_lookup::Invalid swid number\n");
      return NULL;
   }
   return &spacc->job[spacc->job_lookup[swid]];
}

