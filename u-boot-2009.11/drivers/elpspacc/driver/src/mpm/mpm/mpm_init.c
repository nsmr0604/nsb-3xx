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

int mpm_init(mpm_device *mpm, void *regmap, spacc_device *spacc, int no_chains, int no_pdus, int no_keys, int no_chain_links, int no_demand)
{
   int x;

   memset(mpm, 0, sizeof *mpm);

   mpm->spacc                 = spacc;
   mpm->regmap                = regmap;
   mpm->config.no_chains      = no_chains;
   mpm->config.no_pdus        = no_pdus;
   mpm->config.no_keys        = no_keys;
   mpm->config.no_chain_links = no_chain_links;
   mpm->config.no_demand      = no_demand;

   mpm->pdu.freepdu          = pdu_malloc(sizeof(int) * no_pdus);
   mpm->pdu.ondemand         = pdu_malloc(sizeof(int) * no_demand);
   mpm->pdu.ondemand_cb      = pdu_malloc(sizeof(mpm_callback) * no_demand);
   mpm->pdu.ondemand_cb_data = pdu_malloc(sizeof(void *) * no_demand);
   mpm->pdu.ondemand_mask    = pdu_malloc(sizeof(int) * no_pdus);
   mpm->key.freekey          = pdu_malloc(sizeof(int) * no_keys);
   mpm->key.key_ref_cnt      = pdu_malloc(sizeof(int) * no_keys);
   mpm->chains               = pdu_malloc(sizeof(mpm_chain) * no_chains);

   if (!mpm->pdu.ondemand_cb_data || !mpm->pdu.freepdu || !mpm->pdu.ondemand_mask || !mpm->key.freekey || !mpm->key.key_ref_cnt || !mpm->chains || !mpm->pdu.ondemand || !mpm->pdu.ondemand_cb) {
      goto ERR;
   }

   for (x = 0; x < no_demand; x++) {
      mpm->pdu.ondemand[x]    = -1;
      mpm->pdu.ondemand_cb[x] = NULL;
   }

   for (x = 0; x < no_chains; x++) {
     mpm->chains[x].pdus          = pdu_malloc(sizeof(int) * no_chain_links);
     mpm->chains[x].keys          = pdu_malloc(sizeof(int) * no_chain_links);
     mpm->chains[x].callbacks     = pdu_malloc(sizeof(mpm_callback) * no_chain_links);
     mpm->chains[x].callback_data = pdu_malloc(sizeof(void *) * no_chain_links);
     if (!mpm->chains[x].pdus || !mpm->chains[x].keys || !mpm->chains[x].callbacks || !mpm->chains[x].callback_data) {
        goto ERR;
     }
     mpm->chains[x].per_chain_callback = NULL;
     mpm->chains[x].state              = CHAIN_FREE;
     mpm->chains[x].curidx             = 0;
   }
   mpm->chain_idx        = 0;
   mpm->active_chain_idx = 0;

   mpm->pdus_mem_req = sizeof(pdubuf) * no_pdus;
   mpm->keys_mem_req = sizeof(keybuf) * no_keys;

   for (x = 0; x < no_pdus; x++) {
      mpm->pdu.freepdu[x]       = x;
      mpm->pdu.ondemand_mask[x] = -1;
   }
   for (x = 0; x < no_keys; x++) {
      mpm->key.freekey[x]     = x;
      mpm->key.key_ref_cnt[x] = 0;
   }
   mpm->pdu.freepdu_idx = no_pdus - 1;
   mpm->key.freekey_idx = no_keys - 1;

   mpm->busy              = 0;
   mpm->work_to_clear     = 0;
   mpm->pdu.ondemand_idx  = 0;

   PDU_INIT_LOCK_BH(&mpm->lock);

   return 0; 
ERR:
   ELPHW_PRINT("mpm_init::Out of memory\n");
   if (mpm->pdu.freepdu)          { pdu_free(mpm->pdu.freepdu); }
   if (mpm->pdu.ondemand)         { pdu_free(mpm->pdu.ondemand); }
   if (mpm->pdu.ondemand_cb)      { pdu_free(mpm->pdu.ondemand_cb); }
   if (mpm->pdu.ondemand_cb_data) { pdu_free(mpm->pdu.ondemand_cb_data); }
   if (mpm->pdu.ondemand_mask)    { pdu_free(mpm->pdu.ondemand_mask); }
   if (mpm->key.freekey)          { pdu_free(mpm->key.freekey); }
   if (mpm->key.key_ref_cnt)      { pdu_free(mpm->key.key_ref_cnt); }
   if (mpm->chains) {
      for (x = 0; x < no_chains; x++) {
         if (mpm->chains[x].pdus)          { pdu_free(mpm->chains[x].pdus); }
         if (mpm->chains[x].keys)          { pdu_free(mpm->chains[x].keys); }
         if (mpm->chains[x].callbacks)     { pdu_free(mpm->chains[x].callbacks); }
         if (mpm->chains[x].callback_data) { pdu_free(mpm->chains[x].callback_data); }
      }
      pdu_free(mpm->chains);
   }
   return -1;
}
 
