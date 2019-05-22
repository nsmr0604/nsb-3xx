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

#include "eape.h"

int eape_open(eape_device *eape)
{
   int x, y;
   unsigned long flags;
   
   PDU_LOCK(&eape->lock, flags);
   
   for (x = 0; x < eape->num_ctx; x++) {
      if (eape->ctx[x].allocated == 0) {
         eape->ctx[x].allocated    = 1;
         eape->ctx[x].job_cnt      = 0;
         eape->ctx[x].dismiss      = 0;
         for (y = 0; y < MAXJOBS; y++) {
            eape->ctx[x].done[y] = 1;
            eape->ctx[x].cb[y]   = NULL;
         }
         PDU_UNLOCK(&eape->lock, flags);
         return x;
      }
   }
   PDU_UNLOCK(&eape->lock, flags);
   return -1;
}