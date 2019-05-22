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

#include "elpmpm.h"

int mpm_alloc_key(mpm_device *mpm)
{
   int x;

   mpm_cleanup(mpm);

   PDU_LOCK_BH(&mpm->lock);

   if (mpm->key.freekey_idx == -1) {
      ELPHW_PRINT("mpm_alloc_key::No more free KEYs\n");
      PDU_UNLOCK_BH(&mpm->lock);
      return -1;
   }

   /* fetch off stack and decrement pointer */
   x = mpm->key.freekey[mpm->key.freekey_idx--];

   PDU_UNLOCK_BH(&mpm->lock);

   return x;
}
