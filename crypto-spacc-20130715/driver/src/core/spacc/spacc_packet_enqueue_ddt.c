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

#include "elpspacc.h"

static int _spacc_fifo_full(spacc_device *spacc, uint32_t prio)
{
   if (spacc->config.is_qos) {
      return pdu_io_read32(spacc->regmap + SPACC_REG_FIFO_STAT) & SPACC_FIFO_STAT_CMDX_FULL(prio);
   } else {
      return pdu_io_read32(spacc->regmap + SPACC_REG_FIFO_STAT) & SPACC_FIFO_STAT_CMD0_FULL;
   }
}

// When proc_sz != 0 it overrides the ddt_len value defined in the context referenced by 'job_idx'
int spacc_packet_enqueue_ddt_ex (spacc_device * spacc, int use_jb, int job_idx, pdu_ddt * src_ddt, pdu_ddt * dst_ddt, uint32_t proc_sz, uint32_t aad_offset, uint32_t pre_aad_sz,
                              uint32_t post_aad_sz, uint32_t iv_offset, uint32_t prio)
{
   int ret = CRYPTO_OK, proc_len;
   spacc_job *job;

   if (job_idx < 0 || job_idx > SPACC_MAX_JOBS) {
      return CRYPTO_INVALID_HANDLE;
   }

   switch (prio)  {
      case SPACC_SW_CTRL_PRIO_MED: if (spacc->config.cmd1_fifo_depth == 0) { return CRYPTO_CMD_FIFO_INACTIVE; }; break;
      case SPACC_SW_CTRL_PRIO_LOW: if (spacc->config.cmd2_fifo_depth == 0) { return CRYPTO_CMD_FIFO_INACTIVE; }; break;
   }

   job = &spacc->job[job_idx];
   if (NULL == job) {
      ret = CRYPTO_FAILED;
   } else {
// If HSM shared mode
      if (spacc->config.is_hsm_shared) {
         ret = spacc_request_hsm_semaphore(spacc);
         if (ret != CRYPTO_OK) {
             return ret;
         }
      }

      if (_spacc_fifo_full(spacc, prio)) {
         if (spacc->config.is_hsm_shared) {
            spacc_free_hsm_semaphore(spacc);
         }
         if (use_jb) {
            goto fifo_full;
         } else {
            return CRYPTO_FIFO_FULL;
         }

      }


      /* compute the length we must process, in decrypt mode with an ICV (hash, hmac or CCM modes)
       * we must subtract the icv length from the buffer size
       */
      if (proc_sz == SPACC_AUTO_SIZE) {
         if ((job->op == OP_DECRYPT) &&
             ((job->hash_mode > 0) || (job->enc_mode == CRYPTO_MODE_AES_CCM || job->enc_mode == CRYPTO_MODE_AES_GCM)) &&
             !(job->ctrl & CTRL_SET_ICV_ENC)) {
            proc_len = src_ddt->len - job->icv_len;
         } else {
            proc_len = src_ddt->len;
         }
      } else {
         proc_len = proc_sz;
      }

      if (pre_aad_sz & SPACC_AADCOPY_FLAG) {
         job->ctrl |= CTRL_SET_AAD_COPY;
         pre_aad_sz &= ~(SPACC_AADCOPY_FLAG);
      } else {
         job->ctrl &= ~(CTRL_SET_AAD_COPY);
      }

      job->pre_aad_sz  = pre_aad_sz;
      job->post_aad_sz = post_aad_sz;

      if (spacc->config.dma_type == SPACC_DMA_DDT) {
         pdu_io_cached_write32 (spacc->regmap + SPACC_REG_SRC_PTR, (uint32_t) src_ddt->phys, &spacc->cache.src_ptr);
         pdu_io_cached_write32 (spacc->regmap + SPACC_REG_DST_PTR, (uint32_t) dst_ddt->phys, &spacc->cache.dst_ptr);
      } else if (spacc->config.dma_type == SPACC_DMA_LINEAR) {
         pdu_io_cached_write32 (spacc->regmap + SPACC_REG_SRC_PTR, (uint32_t) src_ddt->virt[0], &spacc->cache.src_ptr);
         pdu_io_cached_write32 (spacc->regmap + SPACC_REG_DST_PTR, (uint32_t) dst_ddt->virt[0], &spacc->cache.dst_ptr);
      } else {
         ELPHW_PRINT("Invalid spacc->dma_type variable: %d\n", (int)spacc->config.dma_type);
         return CRYPTO_FAILED;
      }



      if (spacc->config.is_hsm_shared) {
         pdu_io_write32 (spacc->regmap + SPACC_REG_PROC_LEN,     proc_len - job->post_aad_sz);
         pdu_io_write32 (spacc->regmap + SPACC_REG_ICV_LEN,      job->icv_len);
         pdu_io_write32 (spacc->regmap + SPACC_REG_ICV_OFFSET,   job->icv_offset);
         pdu_io_write32 (spacc->regmap + SPACC_REG_PRE_AAD_LEN,  job->pre_aad_sz);
         pdu_io_write32 (spacc->regmap + SPACC_REG_POST_AAD_LEN, job->post_aad_sz);
         pdu_io_write32 (spacc->regmap + SPACC_REG_IV_OFFSET,    iv_offset);
         pdu_io_write32 (spacc->regmap + SPACC_REG_OFFSET,       aad_offset);
         pdu_io_write32 (spacc->regmap + SPACC_REG_AUX_INFO,
                         AUX_INFO_SET_DIR(job->auxinfo_dir)
                         | AUX_INFO_SET_BIT_ALIGN(job->auxinfo_bit_align)
                         | AUX_INFO_SET_CBC_CS(job->auxinfo_cs_mode));
      } else {
         pdu_io_cached_write32 (spacc->regmap + SPACC_REG_PROC_LEN,     proc_len - job->post_aad_sz, &spacc->cache.proc_len);
         pdu_io_cached_write32 (spacc->regmap + SPACC_REG_ICV_LEN,      job->icv_len               , &spacc->cache.icv_len);
         pdu_io_cached_write32 (spacc->regmap + SPACC_REG_ICV_OFFSET,   job->icv_offset            , &spacc->cache.icv_offset);
         pdu_io_cached_write32 (spacc->regmap + SPACC_REG_PRE_AAD_LEN,  job->pre_aad_sz            , &spacc->cache.pre_aad);
         pdu_io_cached_write32 (spacc->regmap + SPACC_REG_POST_AAD_LEN, job->post_aad_sz           , &spacc->cache.post_aad);
         pdu_io_cached_write32 (spacc->regmap + SPACC_REG_IV_OFFSET,    iv_offset                  , &spacc->cache.iv_offset);
         pdu_io_cached_write32 (spacc->regmap + SPACC_REG_OFFSET,       aad_offset                 , &spacc->cache.offset);
         pdu_io_cached_write32 (spacc->regmap + SPACC_REG_AUX_INFO,
                         AUX_INFO_SET_DIR(job->auxinfo_dir)
                         | AUX_INFO_SET_BIT_ALIGN(job->auxinfo_bit_align)
                         | AUX_INFO_SET_CBC_CS(job->auxinfo_cs_mode), &spacc->cache.aux);
      }

      if (job->first_use == 1) {
         pdu_io_write32 (spacc->regmap + SPACC_REG_KEY_SZ, job->ckey_sz | SPACC_SET_KEY_CTX(job->ctx_idx));
         pdu_io_write32 (spacc->regmap + SPACC_REG_KEY_SZ, job->hkey_sz | SPACC_SET_KEY_CTX(job->ctx_idx));
      }

      /* write the job ID to the core, we keep track of it in software now to avoid excessive port I/O */
      pdu_io_write32 (spacc->regmap + SPACC_REG_SW_CTRL, SPACC_SW_CTRL_ID_SET(spacc->job_tally) | SPACC_SW_CTRL_PRIO_SET(prio));
      job->job_swid = (spacc->job_tally)++ & 0xFF;
      spacc->job_tally &= 0xFF;
      spacc->job_lookup[job->job_swid] = job_idx;


#ifdef MAKEAVECTOR
{ int x;
  printk("VEC: Initial cipher context\n");
  for (x = 0; x < spacc->config.ciph_page_size; x += 4) {
     printk("VEC: %08lx\n", pdu_io_read32(spacc->regmap + SPACC_CTX_CIPH_KEY + (spacc->config.ciph_page_size>>2) * job->ctx_idx + x));
  }
  printk("VEC: END\n");
  printk("VEC: Initial hash context\n");
  for (x = 0; x < spacc->config.hash_page_size; x += 4) {
     printk("VEC: %08lx\n", pdu_io_read32(spacc->regmap + SPACC_CTX_HASH_KEY + (spacc->config.hash_page_size>>2) * job->ctx_idx + x));
  }
  printk("VEC: END\n");
}
#endif

      pdu_io_write32 (spacc->regmap + SPACC_REG_CTRL, job->ctrl);

#ifdef MAKEAVECTOR
   printk("VEC: START\n");
   printk("VEC: 00000012    //Opcode\n");
   printk("VEC: %08d     //vSPAcc Indentifier\n", spacc->config.idx);
   printk("VEC: %08lx    //CTRL register\n", pdu_io_read32(spacc->regmap + SPACC_REG_CTRL));
   printk("VEC: %08zx    //source packet length\n", proc_sz + pre_aad_sz + post_aad_sz);
   printk("VEC: %08lx    //Destination packet length\n", 0L);
   printk("VEC: %08lx    //ICV length\n", job->icv_len);
   printk("VEC: %08lx    //ICV offset\n", job->icv_offset);
   printk("VEC: %08lx    //dst/src offset\n", 0L);
   printk("VEC: %08zx    //pre_aad\n", pre_aad_sz);
   printk("VEC: %08lx    //PROC len\n", pdu_io_read32(spacc->regmap + SPACC_REG_PROC_LEN));
   printk("VEC: %08zx    //post_aad\n", post_aad_sz);
   printk("VEC: %08zx    //IV_OFFSET\n", iv_offset);
   printk("VEC: %08lx    //AUX register\n", pdu_io_read32(spacc->regmap + SPACC_REG_AUX_INFO));
   printk("VEC: %08lx    //secure processing cmd\n", 0L);

   printk("VEC: %08lx    //cipher key sz\n", job->ckey_sz | SPACC_SET_KEY_CTX(job->ctx_idx));
   printk("VEC: %08lx    //hash key sz\n",  job->hkey_sz | SPACC_SET_KEY_CTX(job->ctx_idx));
   printk("VEC: END\n");
#endif

// If HSM shared mode
// clear the RQST bit
      if (spacc->config.is_hsm_shared) {
         spacc_free_hsm_semaphore(spacc);
      }

      // Clear an expansion key after the first call
      if (job->first_use == 1) {
         job->first_use = 0;
         job->ctrl &= ~CTRL_SET_KEY_EXP;
      }

   }
   return ret;
fifo_full:
   // try to add a job to the job buffers
{ int i;
   for (i = 0; i < SPACC_MAX_JOB_BUFFERS; i++) {
      if (spacc->job_buffer[i].active == 0) {
         spacc->job_buffer[i] = (struct spacc_job_buffer) {
            .active      = 1,
            .job_idx     = job_idx,
            .src         = src_ddt,
            .dst         = dst_ddt,
            .proc_sz     = proc_sz,
            .aad_offset  = aad_offset,
            .pre_aad_sz  = pre_aad_sz,
            .post_aad_sz = post_aad_sz,
            .iv_offset   = iv_offset,
            .prio        = prio
         };
         spacc->job_buffer_use = 1;
         return 256;
      }
   }
   return CRYPTO_FIFO_FULL;
}

}

int spacc_packet_enqueue_ddt (spacc_device * spacc, int job_idx, pdu_ddt * src_ddt, pdu_ddt * dst_ddt,
                              uint32_t proc_sz, uint32_t aad_offset, uint32_t pre_aad_sz, uint32_t post_aad_sz, uint32_t iv_offset, uint32_t prio)
{
   unsigned long lock_flags;
   int ret;

   PDU_LOCK(&spacc->lock, lock_flags);
   ret = spacc_packet_enqueue_ddt_ex(spacc, 1, job_idx, src_ddt, dst_ddt, proc_sz, aad_offset, pre_aad_sz, post_aad_sz, iv_offset, prio);
   PDU_UNLOCK(&spacc->lock, lock_flags);
   return ret;
}
