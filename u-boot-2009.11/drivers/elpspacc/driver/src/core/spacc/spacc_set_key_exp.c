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

int spacc_set_key_exp(spacc_device *spacc, int job_idx)
{
   spacc_ctx *ctx = NULL;
   spacc_job *job = NULL;

   if (job_idx < 0 || job_idx > SPACC_MAX_JOBS) {
      return CRYPTO_INVALID_HANDLE;
   }

   job = &spacc->job[job_idx];
   ctx = context_lookup_by_job(spacc, job_idx);

   if (!ctx) {
      return CRYPTO_FAILED;
   }

   job->ctrl |= CTRL_SET_KEY_EXP;
   return CRYPTO_OK;
}

