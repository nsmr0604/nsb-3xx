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

int eape_close(eape_device *eape, int handle)
{
   unsigned long flags;
   
   if (handle < 0 || handle > eape->num_ctx) {
      return -1;
   }
   
   PDU_LOCK(&eape->lock, flags);
   
   eape->ctx[handle].dismiss = 1;
   if (!eape->ctx[handle].job_cnt) {
      eape->ctx[handle].allocated = 0;
   }   
   PDU_UNLOCK(&eape->lock, flags);
   return 0;
}