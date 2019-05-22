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
#include "elpkep.h"

int kep_init (void *baseaddr, spacc_device *spacc, kep_device *kep)
{
   int x;

   kep->regmap = baseaddr;
   kep->spacc = spacc;
   PDU_INIT_LOCK (&kep->lock);
   kep->ctx = pdu_malloc ((sizeof (elp_kep_ctx)) * spacc->config.num_ctx);
   if (kep->ctx == NULL) {
      ELPHW_PRINT ("kep_init::Failed to allocate contexts\n");
      return -1;
   }
   memset (kep->ctx, 0, (sizeof (elp_kep_ctx)) * spacc->config.num_ctx);

   for (x = 0; x < spacc->config.num_ctx; x++) {
      kep->ctx[x].job_id = -1;
      kep->ctx[x].job_done = 0;
   }
   return 0;
}
