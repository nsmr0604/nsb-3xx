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

int eape_done(eape_device *eape, int handle, int swid)
{
   int x;
   unsigned long flags, ctx, job;
   
   if (handle < 0 || handle > eape->num_ctx || eape->ctx[handle].allocated == 0 || swid < 0 || swid >= 512) {
      return -1;
   }
   PDU_LOCK(&eape->lock, flags);
   ctx = eape->swid[swid] >> 16;    // handle
   job = eape->swid[swid] & 0xFFFF; // job slot inside context 
   if (ctx > eape->num_ctx || job > MAXJOBS) {
      PDU_UNLOCK(&eape->lock, flags);
      return -1;
   }
   x = eape->ctx[ctx].done[job];
   PDU_UNLOCK(&eape->lock, flags);
   return x;
}