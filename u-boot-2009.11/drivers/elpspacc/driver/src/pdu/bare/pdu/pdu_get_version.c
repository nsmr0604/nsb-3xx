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
#include "elppdu.h"

// ********** NOTICE **********
// this is the unmaintained parallel copy of the pdu_get_version function
// you may wish to copy from the Linux copy for any newer features


// some defines
#define PDU_REG_SPACC_VERSION 0x00180UL
#define PDU_REG_SPACC_CONFIG  0x00184UL
#define PDU_REG_PDU_CONFIG    0x00188UL
#define PDU_REG_SPACC_CONFIG2 0x00190UL

#ifndef SPACC_ID_MINOR
#define SPACC_ID_MINOR(x)   ((x)         & 0x0F)
#define SPACC_ID_MAJOR(x)   (((x) >>  4) & 0x0F)
#define SPACC_ID_QOS(x)     (((x) >>  8) & 0x01)
#define SPACC_ID_TYPE(x)    (((x) >>  9) & 0x03)
#define SPACC_TYPE_SPACCQOS 0
#define SPACC_TYPE_PDU 1
#define SPACC_TYPE_HSM 2
#define SPACC_ID_AUX(x)     (((x) >> 11) & 0x01)
#define SPACC_ID_VIDX(x)    (((x) >> 12) & 0x07)
#define SPACC_ID_PARTIAL(x) (((x) >> 15) & 0x01)
#define SPACC_ID_PROJECT(x) ((x)>>16)

#define SPACC_CFG_CTX_CNT(x)       ((x) & 0x7F)
#define SPACC_CFG_RC4_CTX_CNT(x)   (((x) >> 8) & 0x7F)
#define SPACC_CFG_VSPACC_CNT(x)    (((x) >> 16) & 0x0F)
#define SPACC_CFG_CIPH_CTX_SZ(x)   (((x) >> 20) & 0x07)
#define SPACC_CFG_HASH_CTX_SZ(x)   (((x) >> 24) & 0x07)
#define SPACC_CFG_DMA_TYPE(x)      (((x) >> 28) & 0x03)

#define SPACC_CFG_CMD0_FIFO_QOS(x)   (((x)>>0)&0x7F)
#define SPACC_CFG_CMD0_FIFO(x)   (((x)>>0)&0x1FF)
#define SPACC_CFG_CMD1_FIFO(x)   (((x)>>8)&0x7F)
#define SPACC_CFG_CMD2_FIFO(x)   (((x)>>16)&0x7F)
#define SPACC_CFG_STAT_FIFO(x)   (((x)>>24)&0x7F)


#define SPACC_PDU_CFG_MINOR(x)   ((x) & 0x0F)
#define SPACC_PDU_CFG_MAJOR(x)   (((x)>>4)  & 0x0F)
#define SPACC_PDU_CFG_RNG(x)     (((x)>>8)  & 0x01)
#define SPACC_PDU_CFG_PKA(x)     (((x)>>9)  & 0x01)
#define SPACC_PDU_CFG_RE(x)      (((x)>>10) & 0x01)
#define SPACC_PDU_CFG_KEP(x)     (((x)>>11) & 0x01)
#define SPACC_PDU_CFG_EA(x)      (((x)>>12) & 0x01)
#define SPACC_PDU_CFG_MPM(x)     (((x)>>13) & 0x01)

#define SPACC_HSM_CFG_MINOR(x)   ((x) & 0x0F)
#define SPACC_HSM_CFG_MAJOR(x)   (((x)>>4)  & 0x0F)
#define SPACC_HSM_CFG_PARADIGM(x)    (((x)>>8)  & 0x01)
#define SPACC_HSM_CFG_KEY_CNT(x)  (((x)>>16)&0xFF)
#define SPACC_HSM_CFG_KEY_SZ(x)   (((x)>>14)&0x03)
#endif

// macros for v4.7 and below
#define SPACC_ID_AUX_V47(x)    (((x) >> 10) & 1)
#define SPACC_ID_QOS_V47(x)    (((x) >> 8) & 1)
#define SPACC_ID_PDU_V47(x)    (((x) >> 9) & 1)

#define SPACC_CFG_STAT_FIFO_QOS(x)   (((x)>>24)&0x7F)

// configs for SPAccs v4.7 and lower
static const struct {
   unsigned project;
   spacc_config_block cfg;
} spacc_config[] = {

   { 0x0412, (spacc_config_block){4,0,2,7,6,1,0,0,0,0} },
   { 0x1201, (spacc_config_block){8,4,1,7,7,1,0,0,0,0} },
   { 0x1802, (spacc_config_block){16,16,1,7,6,1,0,0,0,0} },
   { 0x1805, (spacc_config_block){64,0,2,7,6,1,0,0,0,0} },

   // end of list
   { 0, (spacc_config_block){0} },
};

// although the spacc_config_blocks above will have the FIFO depths these are
// used for platforms that are in the v4.8 to v4.10 range since they lack
// FIFO depth config registers
// For anything below v4.11 you must set BOTH structures, you can set the FIFO
// depths above to zero if you wish as these below will overwrite it.
static const struct {
   unsigned project;
   int cmd0_depth,
       cmd1_depth,
       cmd2_depth,
       stat_depth;
} spacc_fifos[] = {

   { 0x0412, 16, 0, 0, 16 },
   { 0x0414,  4, 0, 0,  4 },
   { 0x0601,  8, 0, 0,  8 },
   { 0x0605,  8, 0, 0,  8 },
   { 0x1201,  4, 0, 0,  4 },
   { 0x1802, 16, 0, 0, 16 },
   { 0x1805, 32, 32, 0, 32 },

   { 0x5604,  4,  0, 0,  4 },
   { 0x5407,  4,  0, 0,  4 },

   { 0x9999,  8,  0, 0,  8 },

   // end of list
   { 0 },
};


static int assign_spacc_config_block(unsigned project, pdu_info *inf)
{
   unsigned x;
   for (x = 0; spacc_config[x].project; x++) {
      if (spacc_config[x].project == project) {
         inf->spacc_config = spacc_config[x].cfg;
         return 0;
      }
   }
   return -1;
}

static int assign_spacc_fifo_depth(unsigned project, pdu_info *inf)
{
   unsigned x;
   for (x = 0; spacc_fifos[x].project; x++) {
      if (spacc_fifos[x].project == project) {
         inf->spacc_config.cmd0_fifo_depth = spacc_fifos[x].cmd0_depth;
         inf->spacc_config.cmd1_fifo_depth = spacc_fifos[x].cmd1_depth;
         inf->spacc_config.cmd2_fifo_depth = spacc_fifos[x].cmd2_depth;
         inf->spacc_config.stat_fifo_depth = spacc_fifos[x].stat_depth;
         return 0;
      }
   }
   return -1;
}


int pdu_get_version(void *dev, pdu_info *inf)
{
   unsigned long tmp, ver;

   if (inf == NULL) {
      return -1;
   }

   memset(inf, 0, sizeof *inf);
   tmp = pdu_io_read32(dev + PDU_REG_SPACC_VERSION);

   ver = (SPACC_ID_MAJOR(tmp)<<4)|SPACC_ID_MINOR(tmp);

   if (SPACC_ID_MAJOR(tmp) < 0x04) {
      // we don't support SPAccs before v4.00
      printf("SPAcc MAJOR: %d not supported\n", SPACC_ID_MAJOR(tmp));
      return -1;
   }

// ***** Read the SPAcc version block this tells us the revision, project, and a few other feature bits *****
   if (ver <= 0x47) {
      // older cores had a different layout
      printf("SPACC::Detected older core, not all configuration values may be correct\n");
      inf->spacc_version = (spacc_version_block){SPACC_ID_MINOR(tmp),
                                                 SPACC_ID_MAJOR(tmp),
                                                 (SPACC_ID_MAJOR(tmp)<<4)|SPACC_ID_MINOR(tmp),
                                                 SPACC_ID_QOS_V47(tmp),
                                                 SPACC_ID_PDU_V47(tmp),
                                                 1,
                                                 0,
                                                 SPACC_ID_AUX_V47(tmp),
                                                 SPACC_ID_VIDX(tmp),
                                                 0,
                                                 SPACC_ID_PROJECT(tmp)};
   } else {
      // layout for v4.8+
      inf->spacc_version = (spacc_version_block){SPACC_ID_MINOR(tmp),
                                                 SPACC_ID_MAJOR(tmp),
                                                 (SPACC_ID_MAJOR(tmp)<<4)|SPACC_ID_MINOR(tmp),
                                                 SPACC_ID_QOS(tmp),
                                                 (SPACC_ID_TYPE(tmp) == SPACC_TYPE_SPACCQOS) ? 1:0,
                                                 (SPACC_ID_TYPE(tmp) == SPACC_TYPE_PDU) ? 1:0,
                                                 (SPACC_ID_TYPE(tmp) == SPACC_TYPE_HSM) ? 1:0,
                                                 SPACC_ID_AUX(tmp),
                                                 SPACC_ID_VIDX(tmp),
                                                 SPACC_ID_PARTIAL(tmp),
                                                 SPACC_ID_PROJECT(tmp)};
   }

   if (ver <= 0x4C) {
     inf->spacc_version.ivimport = 2;
   } else {
      // TODO: read register to set it
   }


// ***** Read the SPAcc config block (v4.8+) which tells us how many contexts there are and context page sizes *****
   // this register only available in v4.8 and up
   if (inf->spacc_version.version >= 0x48) {
      tmp = pdu_io_read32(dev + PDU_REG_SPACC_CONFIG);
      inf->spacc_config = (spacc_config_block){SPACC_CFG_CTX_CNT(tmp),
                                               SPACC_CFG_RC4_CTX_CNT(tmp),
                                               SPACC_CFG_VSPACC_CNT(tmp),
                                               SPACC_CFG_CIPH_CTX_SZ(tmp),
                                               SPACC_CFG_HASH_CTX_SZ(tmp),
                                               SPACC_CFG_DMA_TYPE(tmp),0,0,0,0};
   } else {
      // have to lookup based on Project number
      if (assign_spacc_config_block(inf->spacc_version.project, inf)) {
         printf("SPACC::Failed to find configuration block for this device (%04x), driver will likely not work properly\n", inf->spacc_version.project);
         return -1;
      }
   }


// ***** Read the SPAcc config2 block (v4.11) which tells us the FIFO depths of the core *****
   if (inf->spacc_version.version >= 0x4b) {
      // CONFIG2 only present in v4.11+ cores
      tmp = pdu_io_read32(dev + PDU_REG_SPACC_CONFIG2);
      if (inf->spacc_version.qos) {
         inf->spacc_config.cmd0_fifo_depth = SPACC_CFG_CMD0_FIFO_QOS(tmp);
         inf->spacc_config.cmd1_fifo_depth = SPACC_CFG_CMD1_FIFO(tmp);
         inf->spacc_config.cmd2_fifo_depth = SPACC_CFG_CMD2_FIFO(tmp);
         inf->spacc_config.stat_fifo_depth = SPACC_CFG_STAT_FIFO_QOS(tmp);
      } else {
         inf->spacc_config.cmd0_fifo_depth = SPACC_CFG_CMD0_FIFO(tmp);
         inf->spacc_config.stat_fifo_depth = SPACC_CFG_STAT_FIFO(tmp);
      }
   } else {
      if (assign_spacc_fifo_depth(inf->spacc_version.project, inf)) {
         printf("SPACC::Failed to find FIFO depth configuration for this device (%04x), driver will likely not work properly\n", inf->spacc_version.project);
         return -1;
      }
   }

   /* only read PDU config if it's actually a PDU engine */
   if (inf->spacc_version.is_pdu) {
      tmp = pdu_io_read32(dev + PDU_REG_PDU_CONFIG);
      inf->pdu_config = (pdu_config_block){SPACC_PDU_CFG_MINOR(tmp),
                                           SPACC_PDU_CFG_MAJOR(tmp),
                                           SPACC_PDU_CFG_RNG(tmp),
                                           SPACC_PDU_CFG_PKA(tmp),
                                           SPACC_PDU_CFG_RE(tmp),
                                           SPACC_PDU_CFG_KEP(tmp),
                                           SPACC_PDU_CFG_EA(tmp),
                                           SPACC_PDU_CFG_MPM(tmp)};
   }

   /* only read HSM config if it's actually a HSM engine */
   if (inf->spacc_version.is_hsm) {
      tmp = pdu_io_read32(dev + PDU_REG_PDU_CONFIG);
      inf->hsm_config = (hsm_config_block){SPACC_HSM_CFG_MINOR(tmp),
                                           SPACC_HSM_CFG_MAJOR(tmp),
                                           SPACC_HSM_CFG_PARADIGM(tmp),
                                           SPACC_HSM_CFG_KEY_CNT(tmp),
                                           SPACC_HSM_CFG_KEY_SZ(tmp)};
   }

   return 0;
}

