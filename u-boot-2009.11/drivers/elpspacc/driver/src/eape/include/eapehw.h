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

#ifndef EAPEHW_H_
#define EAPEHW_H_

#define EAPE_OK         0
#define EAPE_ERR       -1
#define EAPE_FIFO_FULL -2
#define EAPE_JOBS_FULL -3
#define EAPE_SEQ_ROLL  -4

   #define SA_SIZE               0x80

// registers (shared between EAPE)
#define EAPE_REG_IRQ_EN      0x00
#define EAPE_REG_IRQ_STAT    0x04
#define EAPE_REG_DMA_BURST   0x08
#define EAPE_REG_IV_RND      0x10

#define EAPE_REG_OUT_SRC_PTR 0x20
#define EAPE_REG_OUT_DST_PTR 0x24
#define EAPE_REG_OUT_OFFSET  0x28
#define EAPE_REG_OUT_ID      0x2C
#define EAPE_REG_OUT_SAI     0x30
#define EAPE_REG_OUT_POP     0x38
#define EAPE_REG_OUT_STAT    0x3C

#define EAPE_REG_IN_SRC_PTR  0x40
#define EAPE_REG_IN_DST_PTR  0x44
#define EAPE_REG_IN_OFFSET   0x48
#define EAPE_REG_IN_ID       0x4C
#define EAPE_REG_IN_SAI      0x50
#define EAPE_REG_IN_POP      0x58
#define EAPE_REG_IN_STAT     0x5C

// bitfields
#define EAPE_IRQ_EN_OUT_CMD_EN  0x0001
#define EAPE_IRQ_EN_OUT_STAT_EN 0x0002
#define EAPE_IRQ_EN_IN_CMD_EN   0x0004
#define EAPE_IRQ_EN_IN_STAT_EN  0x0008
#define EAPE_IRQ_EN_GLBL_EN     0x80000000

#define EAPE_IRQ_STAT_OUT_CMD   0x0001
#define EAPE_IRQ_STAT_OUT_STAT  0x0002
#define EAPE_IRQ_STAT_IN_CMD    0x0004
#define EAPE_IRQ_STAT_IN_STAT   0x0008

#define EAPE_OFFSET_SRC         0
#define EAPE_OFFSET_DST         16

#define EAPE_STAT_LENGTH(x)           ((x) & 0xFFFF)
#define EAPE_STAT_ID(x)               (((x)>>16)&0xFF)
#define EAPE_STAT_RET_CODE(x)         (((x)>>24)&0xF)
#define EAPE_STAT_CMD_FIFO_FULL(x)    (((x)>>30)&1)
#define EAPE_STAT_STAT_FIFO_EMPTY(x)  (((x)>>31)&1)


// SA fields
#define EAPE_SA_SEQNUM        0x000
#define EAPE_SA_ANTI_REPLAY   0x008
#define EAPE_SA_AUTH_KEY1     0x010
#define EAPE_SA_CIPH_KEY      0x024
#define EAPE_SA_CIPH_IV       0x044
#define EAPE_SA_SALT          0x044
#define EAPE_SA_AUTH_KEY2     0x048
#define EAPE_SA_REMOTE_SPI    0x054
#define EAPE_SA_HARD_TTL_HI   0x06C
#define EAPE_SA_HARD_TTL_LO   0x070
#define EAPE_SA_SOFT_TTL_HI   0x074
#define EAPE_SA_SOFT_TTL_LO   0x078
#define EAPE_SA_ALG           0x07C
#define EAPE_SA_FLAGS         0x07E

#define EAPE_SA_FLAGS_ACTIVE      0x0001
#define EAPE_SA_FLAGS_SEQ_ROLL    0x0002
#define EAPE_SA_FLAGS_TTL_EN      0x0004
#define EAPE_SA_FLAGS_TTL_CTRL    0x0008
#define EAPE_SA_FLAGS_HDR_TYPE    0x0010
#define EAPE_SA_FLAGS_AR_EN       0x0080
#define EAPE_SA_FLAGS_IPV6        0x1000
#define EAPE_SA_FLAGS_DST_OP_MODE 0x2000
#define EAPE_SA_FLAGS_ESN_EN      0x4000

#define EAPE_SA_ALG_AUTH_NULL      0
#define EAPE_SA_ALG_AUTH_MD5_96    1
#define EAPE_SA_ALG_AUTH_SHA1_96   2
#define EAPE_SA_ALG_AUTH_SHA256    3
#define EAPE_SA_ALG_AUTH_GCM_64    4
#define EAPE_SA_ALG_AUTH_GCM_96    5
#define EAPE_SA_ALG_AUTH_GCM_128   6
#define EAPE_SA_ALG_AUTH_GMAC      7

#define EAPE_SA_ALG_CIPH_NULL       (0<<4)
#define EAPE_SA_ALG_CIPH_DES_CBC    (1<<4)
#define EAPE_SA_ALG_CIPH_3DES_CBC   (2<<4)
#define EAPE_SA_ALG_CIPH_AES128_CBC (3<<4)
#define EAPE_SA_ALG_CIPH_AES192_CBC (4<<4)
#define EAPE_SA_ALG_CIPH_AES256_CBC (5<<4)
#define EAPE_SA_ALG_CIPH_AES128_GCM (6<<4)
#define EAPE_SA_ALG_CIPH_AES192_GCM (7<<4)
#define EAPE_SA_ALG_CIPH_AES256_GCM (8<<4)
#define EAPE_SA_ALG_CIPH_AES128_CTR (9<<4)
#define EAPE_SA_ALG_CIPH_AES192_CTR (10<<4)
#define EAPE_SA_ALG_CIPH_AES256_CTR (11<<4)

#endif
