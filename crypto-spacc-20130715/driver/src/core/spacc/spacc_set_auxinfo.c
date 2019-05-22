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

int spacc_set_auxinfo (spacc_device * spacc, int jobid, uint32_t direction, uint32_t bitsize)
{
   int ret = CRYPTO_OK;
   spacc_job *job;// = job_lookup (spacc, jobid);

   if (jobid < 0 || jobid > SPACC_MAX_JOBS) {
      return CRYPTO_INVALID_HANDLE;
   }

   job = &spacc->job[jobid];
   if (NULL == job) {
      ret = CRYPTO_FAILED;
   } else {
      job->auxinfo_dir = direction;
      job->auxinfo_bit_align = bitsize;
   }
   return ret;
}
