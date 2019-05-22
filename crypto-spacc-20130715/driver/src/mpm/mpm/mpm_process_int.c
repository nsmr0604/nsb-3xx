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

void mpm_process_int_demand(mpm_device *mpm)
{
	int x;

    MPM_PERF_INCR(mpm->perf.demand_irqs, 1);

     // clear all DEMAND jobs pending 
     for (x = 0; x < mpm->config.no_demand; x++) {
        if (mpm->pdu.ondemand[x] != -1) {
           pdubuf *pdu = &mpm->pdu.pdus[mpm->pdu.ondemand[x]];
//           pdu_sync_single_for_cpu(((uint32_t)mpm->pdu.pdus_phys) + mpm->pdu.ondemand[x] * sizeof(pdubuf), sizeof(pdubuf));
           
           if (pdu[0][PDU_DESC_STATUS] & PDU_DESC_STATUS_DONE) {
// subtract time spent per job from start time on x86 
#ifdef MPM_TIMING_X86
  asm __volatile__ ("rdtsc\nsubl (%0),%%eax\nsbbl 4(%0),%%edx\nmovl %%eax,(%0)\nmovl %%edx,4(%0)\n"::"r" (&pdu[0][0x78/4]):"%eax", "%edx", "%cc");
#endif
             mpm->pdu.ondemand_mask[mpm->pdu.ondemand[x]] = -1;    // indicate entry into ondemand as -1 to say it's not an ON DEMAND job anymore
             MPM_PERF_INCR(mpm->perf.demand_caught, 1);
             if (mpm->pdu.ondemand_cb[x] != NULL) {
                mpm->pdu.ondemand_cb[x](&mpm, mpm->pdu.ondemand_cb_data[x], mpm->pdu.ondemand[x], -1, pdu[0][PDU_DESC_STATUS]);
             }
             mpm->pdu.ondemand_cb[x]                     = NULL;  // clear pointer to function which may not be valid 
             mpm->pdu.ondemand[x]                        = -1;    // free up spot in ON DEMAND list
          }
       }
    }
}

void mpm_process_int_eol(mpm_device *mpm)
{
	int x, z;
	MPM_PERF_INCR(mpm->perf.eol_irqs, 1);
	mpm->chains[mpm->active_chain_idx].state = CHAIN_DONE;
	//mpm_sync_chain_for_cpu(&mpm, mpm->active_chain_idx);

	// try to enqueue another chain from the IRQ handler (lower latency if possible) 
	for (z = 0, x = mpm->active_chain_idx + 1; z != mpm->config.no_chains; x++, z++) {
	if (x >= mpm->config.no_chains) { x = 0; }
		if (mpm->chains[x].state == CHAIN_BUILT) {
		// enqueue this chain 
		mpm->chains[x].state  = CHAIN_RUNNING;
		#ifdef MPM_TIMING_X86
		asm __volatile__ ("rdtsc\nsubl (%0),%%eax\nsbbl 4(%0),%%edx\nmovl %%eax,(%0)\nmovl %%edx,4(%0)\n"::"r" (&mpm->chains[x].slottime):"%eax", "%edx", "%cc");
		ELPHW_PRINT(KERN_DEBUG"MPM:Chain took %08zx%08zx cycles from enqueue to running\n", mpm->chains[x].slottime[1], mpm->chains[x].slottime[0]);
		#endif
		mpm->busy             = 2;
		mpm->active_chain_idx = x;
		#ifdef MPM_PERF_MON
		mpm->perf.idle_counter = pdu_io_read32(mpm->regmap + MPM_STAT_CNT_WAIT);
		#endif  
		//mpm_sync_chain_for_device(&mpm, x);
		pdu_io_write32(mpm->regmap + MPM_START, ((uint32_t)mpm->pdu.pdus_phys) + mpm->chains[x].pdus[0] * sizeof(pdubuf));
		break;
		}
	}
	--(mpm->busy); // decrement will hit 0 if nothing was added or 1 if we just enqueued something 
}
