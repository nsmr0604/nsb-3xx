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


#include "elpre.h"
#include "elprehw.h"


static const struct {
   unsigned project,
            fifo_depth,
            sp_ddt,
            ssl3_en,
            tls10_en,
            tls11_en,
            tls12_en,
            dtls10_en,
            dtls12_en;
} re_configs[] = {
   { 0x5604, 4, 32, 1, 1, 1, 1, 1, 1 },

   { 0x605, 16, 32, 1, 1, 1, 1, 1, 1 },
   { 0 },
};

/*
 * Initialize the device, set up all of the register mappings, map the memory
 * and create a blank SA
 */
int re_init (void *baseaddr, spacc_device *spacc, re_device * re)
{
   int i;


   re->regmap = baseaddr;
   PDU_INIT_LOCK (&re->lock);

   re->spacc          = spacc;
   re->config.num_ctx = SPACC_MAX_JOBS;                     // we allow max # of ctx's though there might not be SPAcc contexts
   re->sa_pool_virt   = NULL;
   re->sa_pool_phys   = (PDU_DMA_ADDR_T)0;
   re->sa_pool_size   = RE_SA_SIZE * spacc->config.num_ctx; // only allocate enough SA's for the # of key context pages

   re->fifo_cnt = 1;

   if (spacc->config.pdu_version >= 0x26) {
      uint32_t ver = pdu_io_read32(re->regmap + RE_VERSION);

      re->config.fifo_depth  = RE_VERSION_FIFO_DEPTH(ver);
      re->config.ssl3_en     = RE_VERSION_SSL3_EN(ver);
      re->config.tls10_en    = RE_VERSION_TLS10_EN(ver);
      re->config.tls11_en    = RE_VERSION_TLS11_EN(ver);
      re->config.tls12_en    = RE_VERSION_TLS12_EN(ver);
      re->config.dtls10_en   = RE_VERSION_DTLS10_EN(ver);
      re->config.dtls12_en   = RE_VERSION_DTLS12_EN(ver);
   } else {
      // scan table for FIFO depth
      for (i = 0; re_configs[i].project; i++) {
         if (re_configs[i].project == spacc->config.project) {
            re->config.fifo_depth  = re_configs[i].fifo_depth;
            re->config.ssl3_en     = re_configs[i].ssl3_en;
            re->config.tls10_en    = re_configs[i].tls10_en;
            re->config.tls11_en    = re_configs[i].tls11_en;
            re->config.tls12_en    = re_configs[i].tls12_en;
            re->config.dtls10_en   = re_configs[i].dtls10_en;
            re->config.dtls12_en   = re_configs[i].dtls12_en;
            break;
         }
      }
      if (!re_configs[i].project) {
         ELPHW_PRINT("re_init::Failed to detect RE project number, defaulting to a FIFO depth of 4\n");
         re->config.fifo_depth = 4;
      }
   }

   ELPHW_PRINT("SPAcc-RE Config (EPN 0x%04x, ver 0x%02x)\n", spacc->config.project, spacc->config.pdu_version);
   ELPHW_PRINT("FIFO_DEPTH: %u\n", re->config.fifo_depth);
   ELPHW_PRINT("      SSL3: %u\n", re->config.ssl3_en);
   ELPHW_PRINT("   TLSv1.0: %u\n", re->config.tls10_en);
   ELPHW_PRINT("   TLSv1.1: %u\n", re->config.tls11_en);
   ELPHW_PRINT("   TLSv1.2: %u\n", re->config.tls12_en);
   ELPHW_PRINT("  DTLSv1.0: %u\n", re->config.dtls10_en);
   ELPHW_PRINT("  DTLSv1.2: %u\n", re->config.dtls12_en);

   re->re_contexts = pdu_malloc (re->config.num_ctx * sizeof (re->re_contexts[0]));
   if (!re->re_contexts) {
      ELPHW_PRINT ("re_init::Failed to allocate memory for contexts\n");
      return -1;
   }
   memset (re->re_contexts, 0, re->config.num_ctx * sizeof (re->re_contexts[0]));
   for (i = 0; i < re->config.num_ctx; i++) {
      re->re_contexts[i].spacc_handle = -1;
      re->re_contexts[i].curjob_swid  = -1;
   }

   for (i = 0; i < SPACC_MAX_JOBS; i++) {
      re->jobid_to_ctx[i] = -1;
   }

   memset(&re->cache, 0, sizeof re->cache);

   pdu_io_write32 (re->regmap + RE_SA_PTR,  0);
   pdu_io_write32 (re->regmap + RE_SRC_PTR, 0);
   pdu_io_write32 (re->regmap + RE_DST_PTR, 0);
   pdu_io_write32 (re->regmap + RE_OFFSET,  0);
   pdu_io_write32 (re->regmap + RE_LEN,     0);

   return CRYPTO_OK;
}
