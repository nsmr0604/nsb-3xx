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

int mpm_enqueue_chain_ex(mpm_device *mpm, mpm_chain_callback cb, void *id)
{
   mpm_chain *chain;

   chain = &mpm->chains[mpm->chain_idx];

   // is current chain BUILDING?
   if (chain->state != CHAIN_BUILDING) {
      ELPHW_PRINT("mpm_enqueue_chain::Current chain is not in BUILDING state\n");
      return 0;
   }
   
   if (chain->curidx == 0) {
      // there are no jobs on the chain ... let's ignore the request 
      return 0;
   }

   chain->per_chain_callback = cb;
   chain->per_chain_id       = id;
   chain->state              = CHAIN_BUILT;

#ifdef MPM_TIMING_X86
  asm __volatile__ ("rdtsc\nmovl %%eax,(%0)\nmovl %%edx,4(%0)\n"::"r" (&chain->slottime):"%eax", "%edx");
#endif
   return 0;
}

int mpm_enqueue_chain(mpm_device *mpm, mpm_chain_callback cb, void *id)
{
   int           err;

   PDU_LOCK_BH(&mpm->lock);
   err = mpm_enqueue_chain_ex(mpm, cb, id);
   PDU_UNLOCK_BH(&mpm->lock);
   return err;
}
