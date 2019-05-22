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

#undef ELPHW_PRINT
#define ELPHW_PRINT printk

// handy 32x32 scaled division without dividing... 
static uint32_t scale(uint32_t nom, uint32_t denom)
{
   uint64_t a, b, c, res;
  
   if (!denom) return 0;
      
   // 64-bit multiply by 10000
#if 1
   a   = (uint64_t)nom * 10000;
#else
   // with just shifts in case your platform doesn't support 64x32 mults 
   a   = nom;
   a   = (a<<13) + (a<<10) + (a<<9) + (a<<8) + (a<<4);
#endif   
   b   = denom;
   c   = 1;
   res = 0;
   
   while (b < a) { b <<= 1; c <<= 1; }
   
   while (a >= denom) {
      while (a >= b) { a -= b; res += c; }
      b >>= 1;
      c >>= 1;
   }
   return res;
}

char *mpm_stats(mpm_device *mpm)
{
   uint32_t tally, x, y;
   char *buf, *p;
   
   mpm_cleanup(mpm);
   
   buf = pdu_malloc(65536);
   if (!buf) {
      return NULL;
   }
   memset(buf, 0, 65536);
   p = buf;
   
   p += sprintf(p, "MPM: Performance Monitoring Stats\n");
   p += sprintf(p, "MPM: EOL Interrupts        : %10lu\n", (unsigned long)mpm->perf.eol_irqs);
   p += sprintf(p, "MPM: DEMAND Interrupts     : %10lu\n", (unsigned long)mpm->perf.demand_irqs);
   p += sprintf(p, "MPM: Jobs Inserted         : %10lu\n", (unsigned long)mpm->perf.job_inserted);
   p += sprintf(p, "MPM: Jobs Cleared          : %10lu\n", (unsigned long)mpm->perf.job_cleared);
   p += sprintf(p, "\nMPM: KEY Full Cache Hit    : %10lu\n", (unsigned long)mpm->perf.key_full_hit);
   p += sprintf(p, "MPM: KEY Part Cache Hit    : %10lu\n", (unsigned long)mpm->perf.key_partial_hit);
   p += sprintf(p, "MPM: KEY Miss              : %10lu\n", (unsigned long)mpm->perf.key_miss);
   p += sprintf(p, "\nMPM: DEMAND jobs hit       : %10lu\n", (unsigned long)mpm->perf.demand_caught);
   p += sprintf(p, "MPM: DEMAND downgrade      : %10lu (job posted as non-demand)\n", (unsigned long)mpm->perf.demand_downgrade);
   p += sprintf(p, "MPM: DEMAND missed         : %10lu (normal EOL IRQ picked it up)\n", (unsigned long)mpm->perf.demand_missed);
   tally = mpm->perf.demand_downgrade + mpm->perf.demand_missed + mpm->perf.demand_caught;
   if (tally) {
      tally = scale(mpm->perf.demand_caught, tally);
      p += sprintf(p, "MPM: DEMAND hitrate        : %7lu.%02lu %%\n", (unsigned long)tally/100, (unsigned long)tally%100);
   }
   p += sprintf(p, "\nMPM: active_total          : %10llu\n", (unsigned long long)mpm->perf.active_counter);
   p += sprintf(p, "MPM: spacc_total           : %10llu\n", (unsigned long long)mpm->perf.spacc_counter);
   p += sprintf(p, "MPM: idle_total            : %10llu\n", (unsigned long long)mpm->perf.idle_counter);
   p += sprintf(p, "MPM: IRQ latency           : %10llu\n", (unsigned long long)mpm->perf.irq_counter);
   tally = mpm->perf.active_counter + mpm->perf.idle_counter;
   if (tally) {
      tally = scale(mpm->perf.active_counter, tally);
      p += sprintf(p, "MPM: active rate           : %5lu.%02lu %%\n", (unsigned long)tally/100, (unsigned long)tally%100);
   }
   tally = mpm->perf.active_counter;
   if (tally) {
      tally = scale(mpm->perf.spacc_counter, tally);
      p += sprintf(p, "MPM: spacc rate            : %5lu.%02lu %%\n", (unsigned long)tally/100, (unsigned long)tally%100);
   }
   tally = mpm->perf.idle_counter;
   if (tally) {
      tally = scale(mpm->perf.irq_counter, tally);
      p += sprintf(p, "MPM: IRQ rate              : %5lu.%02lu %%\n", (unsigned long)tally/100, (unsigned long)tally%100);
   }
   if (mpm->perf.eol_irqs) {
      p += sprintf(p, "MPM: Active Cycles per EOL : %10lu (average)\n", (unsigned long)scale(mpm->perf.active_counter, mpm->perf.eol_irqs)/10000);
   }
   
#if 1
   for (x = 0; x < mpm->config.no_chains; x++) {
      if (mpm->chains[x].curidx) {
         p += sprintf(p, "MPM: chain[%d].state  == %d\n", x, mpm->chains[x].state);
         p += sprintf(p, "MPM: chain[%d].curidx == %d\n", x, mpm->chains[x].curidx);
         for (y = 0; y < mpm->chains[x].curidx; y++) {
            pdubuf *pdu = &mpm->pdu.pdus[mpm->chains[x].pdus[y]];
            p += sprintf(p, "MPM: chain[%d].job[%d].status == %08lx, (this@ %08lx => next@ %08lx, key@ %08lx) \n", x, y, (unsigned long)pdu[0][PDU_DESC_STATUS], (unsigned long)mpm->pdu.pdus_phys + mpm->chains[x].pdus[y] * 0x80, (unsigned long)pdu[0][PDU_DESC_NEXT_PDU], (unsigned long)pdu[0][PDU_DESC_KEY_PTR]);
         }
      }
   }
   p += sprintf(p, "MPM: chain_idx        == %d\n", mpm->chain_idx);
   p += sprintf(p, "MPM: active_chain_idx == %d\n", mpm->active_chain_idx);
   p += sprintf(p, "MPM: busy             == %d\n", mpm->busy);
   p += sprintf(p, "MPM: work_to_clear    == %d\n", mpm->work_to_clear);
   p += sprintf(p, "MPM: pdu/key stack pointers %d/%d\n", mpm->pdu.freepdu_idx, mpm->key.freekey_idx);
#endif  
   return buf;
}
