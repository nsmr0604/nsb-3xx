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

int mpm_enqueue_chain_ex(mpm_device *mpm, mpm_chain_callback cb, void *id);

static void load_bytes(uint32_t *dst, unsigned char *src)
{
   int x;
   uint32_t t;

   for (x = t = 0; x < 16; x++) {
      t = (t << 8) | *src++;
      if ((x & 3) == 3) {
// endianess dependent, the first path is for little and the 2nd for big
#if 1
         *dst++ = htonl(t);
#else
         *dst++ = t;
#endif
         t = 0;
      }
   }
}

int mpm_insert_pdu(mpm_device *mpm, int pdu_idx, int key_idx, int ondemand,
                   mpm_callback cb, void *cb_data,
                   pdu_ddt *src, pdu_ddt *dst, uint32_t pre_aad_len, uint32_t post_aad_len, uint32_t proc_len,
                   uint32_t icv_len, uint32_t icv_offset, uint32_t iv_offset, uint32_t aux_info, uint32_t ctrl,
                   unsigned char *ciph_iv, unsigned char *hash_iv)
{
  int x, err;
  pdubuf *pdu;
  mpm_chain *chain;

  err = -1;

  mpm_cleanup(mpm);

  PDU_LOCK_BH(&mpm->lock);

retry:
  // is current chain marked as building?
  if (mpm->chains[mpm->chain_idx].state == CHAIN_FREE) {
     mpm->chains[mpm->chain_idx].state = CHAIN_BUILDING;
  } else if (mpm->chains[mpm->chain_idx].state != CHAIN_BUILDING) {
    // find a free one
    for (x = 0; x < mpm->config.no_chains; ++x) {
       if (mpm->chains[x].state == CHAIN_FREE) {
          mpm->chains[x].state  = CHAIN_BUILDING;
          mpm->chains[x].curidx = 0;
          mpm->chain_idx = x;
          x = -1;
          break;
       }
    }
    if (x == mpm->config.no_chains) {
       ELPHW_PRINT("mpm_insert_pdu::There are no free chains to insert new jobs into\n");
       goto ERR;
    }
  }

  // is there space in the building chain to add it?
  if (mpm->chains[mpm->chain_idx].curidx >= mpm->config.no_chain_links) {
     // let's do an enqueue and see if there is a free chain
     ELPHW_PRINT(ELPHW_PRINT_DEBUG "mpm_insert_pdu:: Enqueing full chain\n");
     err = mpm_enqueue_chain_ex(mpm, NULL, NULL);
     if (err < 0) {
        goto ERR;
     } else {
        mpm_kernel_schedule_tasklet_by_num(mpm->config.id);
        goto retry;
     }
  }

//printk("mpm_insert_pdu::filling out pdu desc\n");

  // now add it to the chain
  pdu   = &mpm->pdu.pdus[pdu_idx];
#if 0
  ELPHW_PRINT("MPM: Physical address of PDU 0x%08lx\n", (unsigned long)mpm->pdu.pdus_phys + pdu_idx * 128);
#endif

  chain = &mpm->chains[mpm->chain_idx];
  chain->pdus[chain->curidx]      = pdu_idx;
  chain->keys[chain->curidx]      = key_idx;

  // fill out fields ...
  if (mpm->spacc->config.dma_type == SPACC_DMA_DDT) {
     pdu[0][PDU_DESC_SRC_PTR] = (uint32_t)src->phys;
     pdu[0][PDU_DESC_DST_PTR] = (uint32_t)dst->phys;
  } else if (mpm->spacc->config.dma_type == SPACC_DMA_LINEAR) {
     pdu[0][PDU_DESC_SRC_PTR] = src->virt[0];
     pdu[0][PDU_DESC_DST_PTR] = dst->virt[0];
  } else {
    ELPHW_PRINT("mpm_insert_pdu::Invalid SPAcc DMA type in SPAcc driver\n");
    goto ERR;
  }
  pdu[0][PDU_DESC_OFFSET]          = 0;
  pdu[0][PDU_DESC_PRE_AAD_LEN]     = pre_aad_len;
  pdu[0][PDU_DESC_POST_AAD_LEN]    = post_aad_len;
  pdu[0][PDU_DESC_PROC_LEN]        = proc_len;
  pdu[0][PDU_DESC_ICV_LEN]         = icv_len;
  pdu[0][PDU_DESC_ICV_OFFSET]      = icv_offset;
  pdu[0][PDU_DESC_IV_OFFSET]       = iv_offset;
  pdu[0][PDU_DESC_AUX_INFO]        = aux_info;
  pdu[0][PDU_DESC_CTRL]            = ctrl;
  pdu[0][PDU_DESC_KEY_PTR]         = ((uint32_t)mpm->key.keys_phys) + key_idx * sizeof(keybuf);
  pdu[0][PDU_DESC_NEXT_PDU]        = 0;
  if (ciph_iv) {
     load_bytes(&pdu[0][PDU_DESC_CIPH_IV], ciph_iv);
  }
  if (hash_iv) {
     load_bytes(&pdu[0][PDU_DESC_HASH_IV], hash_iv);
  }
  pdu[0][PDU_DESC_STATUS]          = 0;

  if (ondemand && mpm->pdu.ondemand[mpm->pdu.ondemand_idx] == -1) {
     pdu[0][PDU_DESC_CTRL]                           |=     (1UL<<_PDU_DESC_CTRL_DEMAND);
     mpm->pdu.ondemand_mask[pdu_idx]                  = mpm->pdu.ondemand_idx;
     mpm->pdu.ondemand_cb[mpm->pdu.ondemand_idx]      = cb;
     mpm->pdu.ondemand_cb_data[mpm->pdu.ondemand_idx] = cb_data;
     mpm->pdu.ondemand[mpm->pdu.ondemand_idx++]       = pdu_idx;
     if (mpm->pdu.ondemand_idx == mpm->config.no_demand) {
        mpm->pdu.ondemand_idx = 0;
     }
     chain->callbacks[chain->curidx]     = NULL;
  } else {
     // we could get here because the job is not an ON DEMAND job or the demand queue is full
     if (ondemand) {
        MPM_PERF_INCR(mpm->perf.demand_downgrade, 1);
     }
     chain->callbacks[chain->curidx]     = cb;
     chain->callback_data[chain->curidx] = cb_data;
     mpm->pdu.ondemand_mask[pdu_idx]     = -1;
  }

  // update ref count on key
  ++(mpm->key.key_ref_cnt[key_idx]);

// register timestamp using borrowed words from PDU buf we assume 0x78 and 0x7C are not used
#ifdef MPM_TIMING_X86
  asm __volatile__ ("rdtsc\nmovl %%eax,(%0)\nmovl %%edx,4(%0)\n"::"r" (&pdu[0][0x78/4]):"%eax", "%edx");
#endif

#if 0
  { int y;
    for (y = 0; y < 0x80/4; y++) {
       ELPHW_PRINT(KERN_DEBUG "PDU[0x%02x] == %08zx\n", y*4, pdu[0][y]);
    }
  }
#endif


  // if this is not the first job in the chain link it
  if (chain->curidx) {
     pdu = &mpm->pdu.pdus[chain->pdus[chain->curidx - 1]];
     pdu[0][PDU_DESC_NEXT_PDU] = ((uint32_t)mpm->pdu.pdus_phys) + chain->pdus[chain->curidx] * sizeof(pdubuf);

     if ((pdu[0][PDU_DESC_NEXT_PDU] < ((uint32_t)mpm->pdu.pdus_phys)) ||
         (pdu[0][PDU_DESC_NEXT_PDU] > ((uint32_t)mpm->pdu.pdus_phys + mpm->config.no_pdus * sizeof(pdubuf)))) {
         ELPHW_PRINT("MPM: INVALID NEXT POINTER WRITTEN OUT %08zx\n", pdu[0][PDU_DESC_NEXT_PDU]);
     }
  }
  ++(chain->curidx);
  MPM_PERF_INCR(mpm->perf.job_inserted, 1);

  err = 0;
ERR:
  PDU_UNLOCK_BH(&mpm->lock);
  return err;
}

