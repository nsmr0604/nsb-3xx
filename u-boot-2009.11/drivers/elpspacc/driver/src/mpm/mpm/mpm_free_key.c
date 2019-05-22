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

/* We decrement the ref count so when clear_chains goes to town and 
 * also decrements the ref count and hits -1 it'll add the key_idx back
 * to the stack of free keys
 **/
int mpm_free_key(mpm_device *mpm, int key_idx)
{
   PDU_LOCK_BH(&mpm->lock);

   /* decerement ref_cnt */
   --(mpm->key.key_ref_cnt[key_idx]);

   if (mpm->key.key_ref_cnt[key_idx] == -1) {
      mpm->key.freekey[++(mpm->key.freekey_idx)] = key_idx;
      mpm->key.key_ref_cnt[key_idx]              = 0;
   }

   PDU_UNLOCK_BH(&mpm->lock);

   return 0;
}
