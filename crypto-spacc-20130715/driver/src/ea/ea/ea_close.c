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

int ea_close(ea_device *ea, int handle)
{
   unsigned long flags;
   
   if (handle < 0 || handle > ea->num_ctx) {
      return -1;
   }
   
   PDU_LOCK(&ea->spacc->lock, flags);
   
   ea->ctx[handle].dismiss = 1;
   if (!ea->ctx[handle].job_cnt) {
      ea->ctx[handle].allocated = 0;
      if (ea->ctx[handle].spacc_handle >= 0) {
         spacc_ctx_release (ea->spacc, ea->ctx[handle].spacc_handle);
         ea->ctx[handle].spacc_handle = -1;
      }
   }   
   PDU_UNLOCK(&ea->spacc->lock, flags);
   return 0;
}
