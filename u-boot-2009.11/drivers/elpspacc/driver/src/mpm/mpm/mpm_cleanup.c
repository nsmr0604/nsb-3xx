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

void mpm_cleanup(mpm_device *mpm)
{
  int x, y;

  if (!mpm->work_to_clear) {
     return;
  }
  PDU_LOCK_BH(&mpm->lock);
  for (x = 0; x < mpm->config.no_chains; ++x) {
      if (mpm->chains[x].state == CHAIN_PROCESSING) {
          --(mpm->work_to_clear);
          for (y = 0; y < mpm->chains[x].curidx; y++) {
              /* put PDU back on stack */
              mpm->pdu.freepdu[++(mpm->pdu.freepdu_idx)] = mpm->chains[x].pdus[y];

              /* decrement key counter (and put on stack if necessary) */
             --(mpm->key.key_ref_cnt[mpm->chains[x].keys[y]]);
             if (mpm->key.key_ref_cnt[mpm->chains[x].keys[y]] == -1) {
                /* push keyidx onto stack of free keys */
                mpm->key.freekey[++(mpm->key.freekey_idx)]   = mpm->chains[x].keys[y];
                mpm->key.key_ref_cnt[mpm->chains[x].keys[y]] = 0; 
             }
        }
        mpm->chains[x].curidx = 0;
        mpm->chains[x].state  = CHAIN_FREE;
     }
  }
  PDU_UNLOCK_BH(&mpm->lock);
}

