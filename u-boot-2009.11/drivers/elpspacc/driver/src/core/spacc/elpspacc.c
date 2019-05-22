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

int spacc_init (void *baseaddr, spacc_device * spacc, pdu_info * info)
{
   unsigned long id;

   if (baseaddr == NULL) {
      ELPHW_PRINT("spacc_init:: baseaddr is NULL\n");
      return -1;
   }
   if (spacc == NULL) {
      ELPHW_PRINT("spacc_init:: spacc is NULL\n");
      return -1;
   }

   memset (spacc, 0, sizeof *spacc);
   PDU_INIT_LOCK(&spacc->lock);
   PDU_INIT_LOCK(&spacc->ctx_lock);

   // assign the baseaddr
   spacc->regmap = baseaddr;

   // version info
   spacc->config.version     = info->spacc_version.version;
   spacc->config.pdu_version = (info->pdu_config.major << 4) | info->pdu_config.minor;
   spacc->config.project     = info->spacc_version.project;
   spacc->config.is_pdu      = info->spacc_version.is_pdu;
   spacc->config.is_qos      = info->spacc_version.qos;

   // hsm specific
   spacc->config.is_hsm_virtual    = info->spacc_version.is_hsm & info->hsm_config.paradigm;
   spacc->config.is_hsm_shared     = info->spacc_version.is_hsm & !(info->hsm_config.paradigm);
   spacc->config.num_sec_ctx       = info->hsm_config.num_ctx;
   spacc->config.sec_ctx_page_size = 4*(1U<<(info->hsm_config.ctx_page_size+2));

   // misc
   spacc->config.is_partial        = info->spacc_version.partial;
   spacc->config.num_ctx           = info->spacc_config.num_ctx;
   spacc->config.num_rc4_ctx       = info->spacc_config.num_rc4_ctx;
   spacc->config.ciph_page_size    = 1U << info->spacc_config.ciph_ctx_page_size;
   spacc->config.hash_page_size    = 1U << info->spacc_config.hash_ctx_page_size;
   spacc->config.dma_type          = info->spacc_config.dma_type;
   spacc->config.idx               = info->spacc_version.vspacc_idx;
   spacc->config.cmd0_fifo_depth   = info->spacc_config.cmd0_fifo_depth;
   spacc->config.cmd1_fifo_depth   = info->spacc_config.cmd1_fifo_depth;
   spacc->config.cmd2_fifo_depth   = info->spacc_config.cmd2_fifo_depth;
   spacc->config.stat_fifo_depth   = info->spacc_config.stat_fifo_depth;
   spacc->config.fifo_cnt          = 1;

   if (info->spacc_version.ivimport != 2) {
      spacc->config.is_ivimport = info->spacc_version.ivimport;
   } else {
      // try to autodetect
      pdu_io_write32(baseaddr + SPACC_REG_IV_OFFSET, 0x80000000);
      if (pdu_io_read32(baseaddr + SPACC_REG_IV_OFFSET) == 0x80000000) {
         spacc->config.is_ivimport = 1;
      } else {
         spacc->config.is_ivimport = 0;
      }
   }

   spacc->job_tally                = 0;
   spacc->wdcnt                    = 0;

   // version 4.10 uses IRQ, above uses WD and we don't support below 4.00
   if (spacc->config.version < 0x40) {
      ELPHW_PRINT("spacc_init::Unsupported SPAcc version\n");
      return CRYPTO_FAILED;
   } else if (spacc->config.version < 0x4B) {
      spacc->op_mode                  = SPACC_OP_MODE_IRQ;
   } else {
      spacc->op_mode                  = SPACC_OP_MODE_WD;
   }

   {
     int x;
     spacc->config.ctx_mask = 0;
     for (x = 1; x < spacc->config.num_ctx; x <<= 1) {
       spacc->config.ctx_mask |= x;
     }
   }

   if (spacc->config.is_hsm_virtual || spacc->config.is_hsm_shared) {
      spacc->config.is_secure      = 1;
      spacc->config.is_secure_port = spacc->config.idx;
   } else {
      spacc->config.is_secure      = 0;
      spacc->config.is_secure_port = 0;
   }

/* set threshold and enable irq */
   // on 4.11 and newer cores we can derive this from the HW reported depths.
   if (spacc->config.stat_fifo_depth == 1) {
      spacc->config.ideal_stat_level = 1;
   } else if (spacc->config.stat_fifo_depth <= 4) {
      spacc->config.ideal_stat_level = spacc->config.stat_fifo_depth - 1;
   } else if (spacc->config.stat_fifo_depth <= 8) {
      spacc->config.ideal_stat_level = spacc->config.stat_fifo_depth - 2;
   } else {
      spacc->config.ideal_stat_level = spacc->config.stat_fifo_depth - 4;
   }

/* determine max PROClen value */
   pdu_io_write32(spacc->regmap + SPACC_REG_PROC_LEN, 0xFFFFFFFF);
   spacc->config.max_msg_size = pdu_io_read32(spacc->regmap + SPACC_REG_PROC_LEN);

   // read config info
   if (spacc->config.is_pdu) {
      ELPHW_PRINT("PDU:\n");
      ELPHW_PRINT("   MAJOR      : %u\n", info->pdu_config.major);
      ELPHW_PRINT("   MINOR      : %u\n", info->pdu_config.minor);
   }
   id = pdu_io_read32 (spacc->regmap + SPACC_REG_ID);
   ELPHW_PRINT ("SPACC ID: (%08lx)\n   MAJOR      : %x\n", (unsigned long) id, info->spacc_version.major);
   ELPHW_PRINT ("   MINOR      : %x\n", info->spacc_version.minor);
   ELPHW_PRINT ("   QOS        : %x\n", info->spacc_version.qos);
   ELPHW_PRINT ("   IVIMPORT   : %x\n", spacc->config.is_ivimport);
   if (spacc->config.version >= 0x48) {
      ELPHW_PRINT ("   TYPE       : %lx (%s)\n", SPACC_ID_TYPE (id), (char *[]){"SPACC", "SPACC-PDU", "SPACC-HSM", "Unknown"}[SPACC_ID_TYPE (id)&3]);
   }
   ELPHW_PRINT ("   AUX        : %x\n", info->spacc_version.qos);
   ELPHW_PRINT ("   IDX        : %lx %s\n", SPACC_ID_VIDX (id), spacc->config.is_secure ? ((char *[]){"(Normal Port)", "(Secure Port)"}[spacc->config.is_secure_port&1]) : "");
   ELPHW_PRINT ("   PARTIAL    : %x\n", info->spacc_version.partial);
   ELPHW_PRINT ("   PROJECT    : %x\n", info->spacc_version.project);
   if (spacc->config.version >= 0x48) {
      id = pdu_io_read32 (spacc->regmap + SPACC_REG_CONFIG);
   } else {
      id = 0xFFFFFFFF;
   }
   ELPHW_PRINT ("SPACC CFG: (%08lx)\n", id);
   ELPHW_PRINT ("   CTX CNT    : %u\n", info->spacc_config.num_ctx);
   ELPHW_PRINT ("   RC4 CNT    : %u\n", info->spacc_config.num_rc4_ctx);
   ELPHW_PRINT ("   VSPACC CNT : %u\n", info->spacc_config.num_vspacc);
   ELPHW_PRINT ("   CIPH SZ    : %-3lu bytes\n", 1UL<<info->spacc_config.ciph_ctx_page_size);
   ELPHW_PRINT ("   HASH SZ    : %-3lu bytes\n", 1UL<<info->spacc_config.hash_ctx_page_size);
   ELPHW_PRINT ("   DMA TYPE   : %u (%s)\n", info->spacc_config.dma_type, (char *[]){"Unknown", "Scattergather", "Linear", "Unknown"}[info->spacc_config.dma_type&3]);
   ELPHW_PRINT ("   MAX PROCLEN: %zu bytes\n", spacc->config.max_msg_size);
   ELPHW_PRINT ("   FIFO CONFIG :\n");
   ELPHW_PRINT ("      CMD0 DEPTH: %d\n", spacc->config.cmd0_fifo_depth);
   if (spacc->config.is_qos) {
      ELPHW_PRINT ("      CMD1 DEPTH: %d\n", spacc->config.cmd1_fifo_depth);
      ELPHW_PRINT ("      CMD2 DEPTH: %d\n", spacc->config.cmd2_fifo_depth);
   }
   ELPHW_PRINT ("      STAT DEPTH: %d\n", spacc->config.stat_fifo_depth);
   if (spacc->config.is_secure) {
      id = pdu_io_read32(spacc->regmap + SPACC_REG_HSM_VERSION);
      ELPHW_PRINT("HSM CFG: (%08lx)\n", id);
      ELPHW_PRINT("   MAJOR     : %x\n", info->hsm_config.major);
      ELPHW_PRINT("   MINOR     : %x\n", info->hsm_config.minor);
      ELPHW_PRINT("   PARADIGM  : %x (%s)\n", info->hsm_config.paradigm, (char *[]){"Shared", "Virtual"}[info->hsm_config.paradigm&1]);
      ELPHW_PRINT("   CTX CNT   : %u (secure key)\n", info->hsm_config.num_ctx);
      ELPHW_PRINT("   CTX SZ    : %-3lu bytes\n", 4*(1UL<<(info->hsm_config.ctx_page_size+2)));
   }

   // Quick sanity check for ptr registers (mask unused bits)
   if (spacc->config.is_hsm_shared && spacc->config.is_secure_port == 0) {
      // request the semaphore
      if (spacc_request_hsm_semaphore(spacc) != CRYPTO_OK) {
         goto ERR;
      }
   }

   if (spacc->config.dma_type == SPACC_DMA_DDT) {
      pdu_io_write32 (baseaddr + SPACC_REG_DST_PTR, 0x1234567F);
      pdu_io_write32 (baseaddr + SPACC_REG_SRC_PTR, 0xDEADBEEF);
      if (((pdu_io_read32 (baseaddr + SPACC_REG_DST_PTR)) != (0x1234567F & SPACC_DST_PTR_PTR)) ||
          ((pdu_io_read32 (baseaddr + SPACC_REG_SRC_PTR)) != (0xDEADBEEF & SPACC_SRC_PTR_PTR))) {
         ELPHW_PRINT("spacc_init::Failed to set pointers\n");
         goto ERR;
      }
   }

   // zero the IRQ CTRL/EN register (to make sure we're in a sane state)
   pdu_io_write32(spacc->regmap + SPACC_REG_IRQ_CTRL,     0);
   pdu_io_write32(spacc->regmap + SPACC_REG_IRQ_EN,       0);
   pdu_io_write32(spacc->regmap + SPACC_REG_IRQ_STAT,     0xFFFFFFFF);

   // init cache
   memset(&spacc->cache, 0, sizeof(spacc->cache));
   pdu_io_write32(spacc->regmap + SPACC_REG_SRC_PTR,      0);
   pdu_io_write32(spacc->regmap + SPACC_REG_DST_PTR,      0);
   pdu_io_write32(spacc->regmap + SPACC_REG_PROC_LEN,     0);
   pdu_io_write32(spacc->regmap + SPACC_REG_ICV_LEN,      0);
   pdu_io_write32(spacc->regmap + SPACC_REG_ICV_OFFSET,   0);
   pdu_io_write32(spacc->regmap + SPACC_REG_PRE_AAD_LEN,  0);
   pdu_io_write32(spacc->regmap + SPACC_REG_POST_AAD_LEN, 0);
   pdu_io_write32(spacc->regmap + SPACC_REG_IV_OFFSET,    0);
   pdu_io_write32(spacc->regmap + SPACC_REG_OFFSET,       0);
   pdu_io_write32(spacc->regmap + SPACC_REG_AUX_INFO,     0);

   // free the HSM
   if (spacc->config.is_hsm_shared && spacc->config.is_secure_port == 0) {
      spacc_free_hsm_semaphore(spacc);
   }

   spacc->ctx = pdu_malloc (sizeof (spacc_ctx) * spacc->config.num_ctx);
   if (spacc->ctx == NULL) {
      ELPHW_PRINT ("spacc_init::Out of memory for ctx\n");
      goto ERR;
   }
   spacc->job = pdu_malloc (sizeof (spacc_job) * SPACC_MAX_JOBS);
   if (spacc->job == NULL) {
      ELPHW_PRINT ("spacc_init::Out of memory for job\n");
      goto ERR;
   }

   /* initialize job_idx and lookup table */
   spacc_job_init_all(spacc);

   /* initialize contexts */
   spacc_ctx_init_all (spacc);

   return CRYPTO_OK;
 ERR:
   spacc_fini (spacc);
   return CRYPTO_FAILED;
}

/* free up the memory */
void spacc_fini (spacc_device * spacc)
{
   pdu_free (spacc->ctx);
   pdu_free (spacc->job);
}

