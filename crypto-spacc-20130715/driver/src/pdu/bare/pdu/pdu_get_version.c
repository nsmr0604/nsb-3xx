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

int pdu_get_version(void *dev, pdu_info *inf)
{
   unsigned long tmp;

   if (inf == NULL) {
      return -1;
   }

   memset(inf, 0, sizeof *inf);
   tmp = pdu_io_read32(dev + PDU_REG_SPACC_VERSION);
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

   tmp = pdu_io_read32(dev + PDU_REG_SPACC_CONFIG);
   inf->spacc_config = (spacc_config_block){SPACC_CFG_CTX_CNT(tmp),
                                            SPACC_CFG_RC4_CTX_CNT(tmp),
                                            SPACC_CFG_VSPACC_CNT(tmp),
                                            SPACC_CFG_CIPH_CTX_SZ(tmp),
                                            SPACC_CFG_HASH_CTX_SZ(tmp),
                                            SPACC_CFG_DMA_TYPE(tmp),0,0,0,0};

   tmp = pdu_io_read32(dev + PDU_REG_SPACC_CONFIG2);
   if (inf->spacc_version.qos) {
      inf->spacc_config.cmd0_fifo_depth = SPACC_CFG_CMD0_FIFO_QOS(tmp);
      inf->spacc_config.cmd1_fifo_depth = SPACC_CFG_CMD1_FIFO(tmp);
      inf->spacc_config.cmd2_fifo_depth = SPACC_CFG_CMD2_FIFO(tmp);
   } else {
      inf->spacc_config.cmd0_fifo_depth = SPACC_CFG_CMD0_FIFO(tmp);
   }
   inf->spacc_config.stat_fifo_depth = SPACC_CFG_STAT_FIFO(tmp);



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
