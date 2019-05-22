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

#include "elpea.h"

int ea_open(ea_device *ea, int lock_ctx)
{
   int x, y;
   unsigned long flags;
   
   PDU_LOCK(&ea->spacc->lock, flags);
   
   for (x = 0; x < ea->num_ctx; x++) {
      if (ea->ctx[x].allocated == 0) {
         ea->ctx[x].allocated    = 1;
         ea->ctx[x].lock_ctx     = lock_ctx;
         ea->ctx[x].spacc_handle = -1;
         ea->ctx[x].job_cnt      = 0;
         ea->ctx[x].dismiss      = 0;
         for (y = 0; y < MAXJOBS; y++) {
            ea->ctx[x].done[y] = 1;
            ea->ctx[x].cb[y]   = NULL;
         }
         PDU_UNLOCK(&ea->spacc->lock, flags);
         return x;
      }
   }
   PDU_UNLOCK(&ea->spacc->lock, flags);
   return -1;
}
