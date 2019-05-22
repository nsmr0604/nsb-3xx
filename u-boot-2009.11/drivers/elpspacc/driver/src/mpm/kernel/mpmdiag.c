
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
#include "elpspaccdrv.h"
#include "elpspacc.h"
#include "elpparser.h"
#include "elpmpm.h"
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/io.h>

#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/sched.h>
#include <linux/platform_device.h>
#include <linux/scatterlist.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>

#include "cs75xx.h"

#ifndef _SPACC_MAX_MSG_MALLOC_SIZE
#define _SPACC_MAX_MSG_MALLOC_SIZE     16
#endif
#define SPACC_MAX_MSG_MALLOC_SIZE     (1 << _SPACC_MAX_MSG_MALLOC_SIZE)

#ifndef SPACC_MAX_MSG_SIZE
#define SPACC_MAX_MSG_SIZE     (SPACC_MAX_MSG_MALLOC_SIZE - 1)
#endif

#define MODE_ENCRYPT 0
#define MODE_HASH    1

// for epn0409, Effective Master Key decrypt/load is forced to context 0.
#define EPN0409_EMK_CTX 0

#define IV_OFFSET_MASK 0x7fffffff
#define IV_OFFSET_EN_MASK 0x80000000

#define CIPH_MODE(ciph, ciphmode) \
   (((ciph)       <<  _PDU_DESC_CTRL_CIPH_ALG)  | \
   ((ciphmode)   <<  _PDU_DESC_CTRL_CIPH_MODE))

#define HASH_MODE(hash, hashmode) \
   (((hash)       <<  _PDU_DESC_CTRL_HASH_ALG)  | \
   ((hashmode)   << _PDU_DESC_CTRL_HASH_MODE))

static uint32_t icv_modes[4] = {
     PDU_DESC_CTRL_ICV_PT,
     PDU_DESC_CTRL_ICV_PT|PDU_DESC_CTRL_ICV_ENC|PDU_DESC_CTRL_ICV_APPEND,
     0,
     0
};

static uint32_t context_modes[2][32] = {
  {
     CIPH_MODE(C_NULL,    CM_ECB),
     CIPH_MODE(C_RC4,     CM_ECB),
     CIPH_MODE(C_RC4,     CM_ECB),
     CIPH_MODE(C_RC4,     CM_ECB),
     CIPH_MODE(C_AES,     CM_ECB),
     CIPH_MODE(C_AES,     CM_CBC),
     CIPH_MODE(C_AES,     CM_CTR),
     CIPH_MODE(C_AES,     CM_CCM),
     CIPH_MODE(C_AES,     CM_GCM),
     CIPH_MODE(C_AES,     CM_F8),
     CIPH_MODE(C_AES,     CM_XTS),
     CIPH_MODE(C_AES,     CM_CFB),
     CIPH_MODE(C_AES,     CM_OFB),
     CIPH_MODE(C_AES,     CM_CBC), //CS1
     CIPH_MODE(C_AES,     CM_CBC), //CS2
     CIPH_MODE(C_AES,     CM_CBC), //CS3
     CIPH_MODE(C_MULTI2,  CM_ECB),
     CIPH_MODE(C_MULTI2,  CM_CBC),
     CIPH_MODE(C_MULTI2,  CM_OFB),
     CIPH_MODE(C_MULTI2,  CM_CFB),
     CIPH_MODE(C_DES,     CM_CBC),
     CIPH_MODE(C_DES,     CM_ECB),
     CIPH_MODE(C_DES,     CM_CBC),
     CIPH_MODE(C_DES,     CM_ECB),
     CIPH_MODE(C_KASUMI,  CM_ECB),
     CIPH_MODE(C_KASUMI,  CM_F8),
     CIPH_MODE(C_SNOW3G_UEA2, CM_ECB),
     CIPH_MODE(C_ZUC_UEA3, CM_ECB),
  },
  {
     HASH_MODE(H_NULL,              HM_RAW),
     HASH_MODE(H_SHA1,              HM_RAW),
     HASH_MODE(H_SHA1,              HM_HMAC),
     HASH_MODE(H_MD5,               HM_RAW),
     HASH_MODE(H_MD5,               HM_HMAC),
     HASH_MODE(H_SHA224,            HM_RAW),
     HASH_MODE(H_SHA224,            HM_HMAC),
     HASH_MODE(H_SHA256,            HM_RAW),
     HASH_MODE(H_SHA256,            HM_HMAC),
     HASH_MODE(H_SHA384,            HM_RAW),
     HASH_MODE(H_SHA384,            HM_HMAC),
     HASH_MODE(H_SHA512,            HM_RAW),
     HASH_MODE(H_SHA512,            HM_HMAC),
     HASH_MODE(H_SHA512_224,        HM_RAW),
     HASH_MODE(H_SHA512_224,        HM_HMAC),
     HASH_MODE(H_SHA512_256,        HM_RAW),
     HASH_MODE(H_SHA512_256,        HM_HMAC),
     HASH_MODE(H_XCBC,              HM_HMAC),
     HASH_MODE(H_CMAC,              HM_HMAC),
     HASH_MODE(H_KF9,               HM_HMAC),
     HASH_MODE(H_MD5,               HM_SSLMAC),
     HASH_MODE(H_SHA1,              HM_SSLMAC),
     HASH_MODE(H_SNOW3G_UIA2,       HM_HMAC),
     HASH_MODE(H_ZUC_UIA3,          HM_HMAC),
     HASH_MODE(H_CRC32_I3E802_3,    HM_HMAC),
  }
};

static int context_tags[2][32] = {
  {CRYPTO_MODE_NULL,
   CRYPTO_MODE_RC4_40,          // 1
   CRYPTO_MODE_RC4_128,
   CRYPTO_MODE_RC4_KS,
   CRYPTO_MODE_AES_ECB,         // 3
   CRYPTO_MODE_AES_CBC,
   CRYPTO_MODE_AES_CTR,
   CRYPTO_MODE_AES_CCM,
   CRYPTO_MODE_AES_GCM,
   CRYPTO_MODE_AES_F8,
   CRYPTO_MODE_AES_XTS,
   CRYPTO_MODE_AES_CFB,
   CRYPTO_MODE_AES_OFB,
   CRYPTO_MODE_AES_CS1,
   CRYPTO_MODE_AES_CS2,
   CRYPTO_MODE_AES_CS3,
   CRYPTO_MODE_MULTI2_ECB,
   CRYPTO_MODE_MULTI2_CBC,
   CRYPTO_MODE_MULTI2_OFB,
   CRYPTO_MODE_MULTI2_CFB,
   CRYPTO_MODE_3DES_CBC,        // 13
   CRYPTO_MODE_3DES_ECB,        // 14
   CRYPTO_MODE_DES_CBC,         // 15
   CRYPTO_MODE_DES_ECB,         // 16
   CRYPTO_MODE_KASUMI_ECB,
   CRYPTO_MODE_KASUMI_F8,
   CRYPTO_MODE_SNOW3G_UEA2,
   CRYPTO_MODE_ZUC_UEA3,
  },
  {  CRYPTO_MODE_NULL,
     CRYPTO_MODE_HASH_SHA1,
     CRYPTO_MODE_HMAC_SHA1,
     CRYPTO_MODE_HASH_MD5,
     CRYPTO_MODE_HMAC_MD5,
     CRYPTO_MODE_HASH_SHA224,
     CRYPTO_MODE_HMAC_SHA224,
     CRYPTO_MODE_HASH_SHA256,
     CRYPTO_MODE_HMAC_SHA256,
     CRYPTO_MODE_HASH_SHA384,
     CRYPTO_MODE_HMAC_SHA384,
     CRYPTO_MODE_HASH_SHA512,
     CRYPTO_MODE_HMAC_SHA512,
     CRYPTO_MODE_HASH_SHA512_224,
     CRYPTO_MODE_HMAC_SHA512_224,
     CRYPTO_MODE_HASH_SHA512_256,
     CRYPTO_MODE_HMAC_SHA512_256,
     CRYPTO_MODE_MAC_XCBC,
     CRYPTO_MODE_MAC_CMAC,
     CRYPTO_MODE_MAC_KASUMI_F9,
     CRYPTO_MODE_SSLMAC_MD5,
     CRYPTO_MODE_SSLMAC_SHA1,
     CRYPTO_MODE_MAC_SNOW3G_UIA2,
     CRYPTO_MODE_MAC_ZUC_UIA3,
     CRYPTO_MODE_HASH_CRC32,
  }
};


enum {
   DIR_ENC=0,
   DIR_DEC
};

static int mpmdiag_verbose=0;
static char *mode_names[][32] = {
  {"NULL",
   "RC4_40",
   "RC4_128",
   "RC4_KS",
   "AES_ECB",
   "AES_CBC",
   "AES_CTR",
   "AES_CCM",
   "AES_GCM",
   "AES_F8",
   "AES_XTS",
   "AES_CFB",
   "AES_OFB",
   "AES_CS1",
   "AES_CS2",
   "AES_CS3",
   "MULTI2_ECB",
   "MULTI2_CBC",
   "MULTI2_OFB",
   "MULTI2_CFB",
   "3DES_CBC",
   "3DES_ECB",
   "DES_CBC",
   "DES_ECB",
   "KASUMI_ECB",
   "KASUMI_F8",
   "SNOW3G_UEA2",
   "ZUC_UEA2",
  },
  {"NULL",
   "HASH_SHA1",
   "HMAC_SHA1",
   "HASH_MD5",
   "HMAC_MD5",
   "HASH_SHA224",
   "HMAC_SHA224",
   "HASH_SHA256",
   "HMAC_SHA256",
   "HASH_SHA384",
   "HMAC_SHA384",
   "HASH_SHA512",
   "HMAC_SHA512",
   "HASH_SHA512_224",
   "HMAC_SHA512_224",
   "HASH_SHA512_256",
   "HMAC_SHA512_256",
   "MAC_XCBC",
   "MAC_CMAC",
   "MAC_KASUMI_F9",
   "SSLMAC_MD5",
   "SSLMAC_SHA1",
   "SNOW3G_UIA2",
   "ZUC_UIA3",
   "CRC32",
 }
};

static void outdiff (unsigned char *buf1, unsigned char *buf2, int len)
{
#if 1
  int x, y;
  char buf[16];
  printk("outdiff len == %d\n", len);
  for (x = 0; x < len; x += 4) {
    if (memcmp(buf1, buf2, 4) == 0) { buf1 += 4; buf2 += 4; continue; }
    sprintf (buf, "%04x   ", x);
    printk (buf);
    for (y = 0; y < 4; y++) {
      sprintf (buf, "%02lx", (unsigned long) buf1[y] & 255UL);
      printk (buf);
    }
    printk (" - ");
    for (y = 0; y < 4; y++) {
      sprintf (buf, "%02lx", (unsigned long) buf2[y] & 255UL);
      printk (buf);
    }
    printk (" = ");
    for (y = 0; y < 4; y++) {
      sprintf (buf, "%02lx", (unsigned long) (buf1[y] ^ buf2[y]) & 255UL);
      printk (buf);
    }
    printk ("\n");
    buf1 += 4;
    buf2 += 4;
    /* exit early */
    //x=len;
  }
#endif
}

/* this function maps a virtual buffer to an SG then allocates a DDT and maps it, it breaks the buffer into 4K chunks */
static int map_buf_to_sg_ddt(const void *src, unsigned long srclen, struct scatterlist **sg, unsigned *sgents, pdu_ddt *ddt)
{
   unsigned x, n, y;
   struct scatterlist *sgtmp;

   *sgents = 0;
   *sg     = NULL;

   n = (srclen+4095) >> 12;
   *sg = kmalloc(n * sizeof(**sg), GFP_KERNEL);
   if (!*sg) {
      printk("Out of memory allocating SG\n");
      return -1;
   }
   sg_init_table(*sg, n);
   for (x = 0; srclen; x++) {
      y = (srclen > 4096) ? 4096 : srclen;
      sg_set_buf(&(*sg)[x], src, y);
      srclen -= y;
      src    += y;
   }
   x = DMA_MAP_SG(NULL, *sg, n, DMA_BIDIRECTIONAL);
   if (!x) {
      kfree(*sg);
      *sg = NULL;
   }
   pdu_ddt_init(ddt, x);
   for_each_sg(*sg, sgtmp, x, y) {
      pdu_ddt_add(ddt, (PDU_DMA_ADDR_T)sg_dma_address(sgtmp), sg_dma_len(sgtmp));
   }
   *sgents = x;
   return 0;
}

static volatile uint32_t job_status;

static void mpmdiag_cb(void *mpm, void *data, int pdu, int key, uint32_t status)
{
   if (PDU_DESC_STATUS_RET_CODE(status) != 0) {
      printk(KERN_DEBUG "Status == %p, %d, %d, %08zx\n", mpm, pdu, key, status);
   }
   job_status = status;
}


static int mpm_vector(char *fname, int dir)
{
   uint32_t     in_buff_size = 0;
   uint32_t     aux_info;
   vector_data *vec;  //the current vector

   mpm_device *mpm;

   int err = 0, y = 0, tries = 0, pdu = -1, key = -1, inv_key, ctx[1];
   uint32_t icvoff = 0;
   uint32_t proc_sz = 0;

   unsigned sgents_src, sgents_dst;
   struct scatterlist *sgsrc, *sgdst;
   pdu_ddt src_ddt, dst_ddt;

   ctx[0] = -1;

   job_status = 0;

   sgents_src = 0;
   sgents_dst = 0;
   sgsrc      = NULL;
   sgdst      = NULL;

   vec = init_test_vector(SPACC_MAX_MSG_MALLOC_SIZE);
   if (vec == NULL) {
      printk("Error allocating and initializing vector data\n");
      return -ENODEV;
   }

   y = -1;
   //use common parser to get the data
   err = parse_test_vector (fname, vec);
   if (err!=0){
      free_test_vector(vec);
      printk("parse_test_vector FAILED!!![%s]\n", fname);
      return y;
   }
vec->virt = 0;
   mpm = mpm_get_device_by_num(vec->virt);

   if (mpm->testing.OUTBUF == NULL) {
      mpm->testing.OUTBUF = kmalloc(SPACC_MAX_MSG_MALLOC_SIZE, GFP_KERNEL);
      if (!mpm->testing.OUTBUF) {
         free_test_vector(vec);
         return -ENOMEM;
      }
   }
   if (mpm->testing.INBUF == NULL) {
      mpm->testing.INBUF = kmalloc(SPACC_MAX_MSG_MALLOC_SIZE, GFP_KERNEL);
      if (!mpm->testing.INBUF) {
         kfree(mpm->testing.OUTBUF);
         free_test_vector(vec);
         return -ENOMEM;
      }
   }
   memset(mpm->testing.INBUF, 0, SPACC_MAX_MSG_MALLOC_SIZE);
   memset(mpm->testing.OUTBUF, 0, SPACC_MAX_MSG_MALLOC_SIZE);


   // allocate context to spacc
   err = mpm_req_spacc_ctx(mpm, 1, ctx);
   if (err) { printk(KERN_DEBUG "mpmdiag: Error allocating a context to the MPM\n"); y = 1; goto ERR; }

   // allocate a pdu and key
   pdu = mpm_alloc_pdu(mpm);
   if (pdu < 0) {
      printk("mpmdiag: Out of PDU handles\n");
      y = 1;
      goto ERR;
   }
   key = mpm_alloc_key(mpm);
   if (key < 0) {
      printk("mpmdiag: Out of KEY handles\n");
      y = 1;
      goto ERR;
   }

   // fill in key context
   inv_key = 0;
   // we embed 3GPP IVs in the key buffer
   if (context_tags[MODE_ENCRYPT][vec->enc_mode] == CRYPTO_MODE_SNOW3G_UEA2 ||
       context_tags[MODE_ENCRYPT][vec->enc_mode] == CRYPTO_MODE_ZUC_UEA3) {
       vec->keysize = 16;
       memcpy(vec->iv, vec->key+16, 16);
   }

   if (context_tags[MODE_HASH][vec->hash_mode] == CRYPTO_MODE_MAC_SNOW3G_UIA2 ||
       context_tags[MODE_HASH][vec->hash_mode] == CRYPTO_MODE_MAC_ZUC_UIA3) {
       vec->hmac_keysize = 16;
   }

   // set the last 4 bytes of the IV since it's not stored in the vector
   if (context_tags[MODE_ENCRYPT][vec->enc_mode] == CRYPTO_MODE_AES_GCM) {
      vec->iv[12] = 0;
      vec->iv[13] = 0;
      vec->iv[14] = 0;
      vec->iv[15] = 1;
   }

   mpm_set_key(mpm, key, vec->keysize, vec->hmac_keysize, 1, inv_key, vec->key, vec->hmac_key);

   /* Add sources */
   if (vec->pre_aad) {
      memcpy(mpm->testing.INBUF, vec->pre_aad, vec->pre_aad_size);
      in_buff_size += vec->pre_aad_size;
   }

   if (dir == DIR_ENC) {
     memcpy(mpm->testing.INBUF + in_buff_size, vec->pt, vec->pt_size);
     in_buff_size += vec->pt_size;
   } else {
      if ((vec->icv_mode != ICV_HASH_ENCRYPT)) {
         memcpy(mpm->testing.INBUF + in_buff_size, vec->ct, vec->pt_size);
         in_buff_size += vec->pt_size;
      } else {
         memcpy(mpm->testing.INBUF + in_buff_size, vec->ct, vec->ct_size);
         in_buff_size += vec->ct_size;
      }
   }

   if (vec->post_aad) {
     memcpy(mpm->testing.INBUF + in_buff_size, vec->post_aad, vec->post_aad_size);
     in_buff_size += vec->post_aad_size;
   }

   if (vec->icv_offset == -1) {
      vec->icvpos = IP_ICV_APPEND;
      icvoff = vec->pre_aad_size + vec->pt_size + vec->post_aad_size;
   } else {
      vec->icvpos = IP_ICV_OFFSET;
      icvoff = vec->icv_offset;
   }

   // in decrypt mode we stick the icv after all this nonsense
   if (vec->icv_size > 0) {
     if (dir == DIR_DEC) {
        if ((vec->icv_mode != ICV_HASH_ENCRYPT)) {
           memcpy(mpm->testing.INBUF + icvoff, vec->icv, vec->icv_size);
           in_buff_size = icvoff + vec->icv_size;
        }
     } else { // dir == DIR_ENC
     }
   } else {
     if ((context_tags[MODE_HASH][vec->hash_mode] != CRYPTO_MODE_NULL)) {
        if ((dir == DIR_DEC) && (vec->ct_size > vec->pt_size)) {
           memcpy(mpm->testing.INBUF + icvoff, vec->ct + vec->pt_size, vec->ct_size - vec->pt_size);
        }
         in_buff_size = icvoff + vec->ct_size - vec->pt_size;
     } else {
     }
   }

   if (vec->iv_offset & IV_OFFSET_EN_MASK) {
     if (in_buff_size < ((vec->iv_offset & IV_OFFSET_MASK) + vec->iv_size)) {
         in_buff_size = (vec->iv_offset & IV_OFFSET_MASK) + vec->iv_size;
     }
      memcpy(mpm->testing.INBUF + (vec->iv_offset & IV_OFFSET_MASK), vec->iv, vec->iv_size);
   }

   if ((err = map_buf_to_sg_ddt(mpm->testing.INBUF, in_buff_size, &sgsrc, &sgents_src, &src_ddt)) != 0) {
     printk("Error mapping source buffer to SG and DDT\n");
     y = 1;
     goto ERR;
   }

   /* Add Destinations */
   if ((err = map_buf_to_sg_ddt(mpm->testing.OUTBUF, SPACC_MAX_MSG_SIZE, &sgdst, &sgents_dst, &dst_ddt)) != 0) {
     printk("Error mapping dest buffer to SG and DDT\n");
     y = 1;
     goto ERR;
   }

   // proclen register
   proc_sz = vec->pre_aad_size + vec->pt_size;

   //add icv_len if we're in ICV_ENC mode
   if ((vec->icv_mode==1) && (dir != DIR_ENC)) {
      proc_sz += vec->icv_size;
   }


   // subtract ICV if in decrypt and using GCM/CCM
#warning Should use table lookup for modes to tags ...
   if ((dir != DIR_ENC) &&
       ((vec->hash_mode > 0) || (vec->enc_mode == CRYPTO_MODE_AES_CCM || vec->enc_mode == CRYPTO_MODE_AES_GCM)) &&
       !(vec->icv_mode & 2)) {
//        proc_sz -= vec->icv_size;
   }

   // auxinfo
   aux_info =  (vec->auxinfo_dir << _SPACC_AUX_INFO_DIR) | (vec->auxinfo_bit_align << _SPACC_AUX_INFO_BIT_ALIGN);

   switch (context_tags[MODE_ENCRYPT][vec->enc_mode]) {
      case CRYPTO_MODE_AES_CS1: aux_info |= AUX_INFO_SET_CBC_CS(1); break;
      case CRYPTO_MODE_AES_CS2: aux_info |= AUX_INFO_SET_CBC_CS(2); break;
      case CRYPTO_MODE_AES_CS3: aux_info |= AUX_INFO_SET_CBC_CS(3); break;
   }

   /* insert pdu */
   err = mpm_insert_pdu(mpm,
             pdu, key, 0, mpmdiag_cb, NULL,
             &src_ddt, &dst_ddt, vec->pre_aad_size, vec->post_aad_size, proc_sz,
             vec->icv_size, (vec->icv_offset == 0xFFFFFFFF) ? 0 : (vec->icv_offset & 0x7FFFFFFF), (vec->iv_offset == 0xFFFFFFFF) ? 0 : (vec->iv_offset & 0x7FFFFFFF),
             aux_info,
             context_modes[0][vec->enc_mode] | context_modes[1][vec->hash_mode] | ((dir == DIR_ENC) ? PDU_DESC_CTRL_ENCRYPT:0) | icv_modes[vec->icv_mode],
             vec->iv, vec->hmac_key + 16);
   if (err < 0) {
      printk("mpmdiag:  Failed to insert vector\n");
      err = 1;
      goto ERR;
   }

   err = mpm_enqueue_chain(mpm, NULL, 0);
   if (err < 0) {
      printk("mpmdiag:  Failed to enqueue vector\n");
      err = 1;
      goto ERR;
   }
   mpm_kernel_schedule_tasklet_by_num(vec->virt);

   tries = 100000;
   while (!job_status && --tries) { yield(); }
   if (!tries) {
      printk("mpmdiag:  Timed out waiting for job\n");
      y = 1;
      goto ERR;
   }
   mpm_free_key(mpm, key);
   key = -1;

   DMA_SYNC_SG_FOR_CPU(NULL, sgdst, sgents_dst, DMA_BIDIRECTIONAL);

   if (PDU_DESC_STATUS_RET_CODE(job_status) != 0) {
      printk("MPM: Vector failed with return code %x\n", PDU_DESC_STATUS_RET_CODE(job_status));
      y = 1;
      goto ERR;
   }


   // zero out unused bits in the last byte returned of the stream cipher for SNOW3G and Kasumi
   if (vec->auxinfo_bit_align != 8 && (vec->hash_mode == CRYPTO_MODE_MAC_KASUMI_F9 || vec->hash_mode == CRYPTO_MODE_MAC_SNOW3G_UIA2) && vec->post_aad_size == 0) {
      if (dir == DIR_ENC) {
         mpm->testing.OUTBUF[vec->ct_size - vec->icv_size - 1] &= 0xFF << (8 - vec->auxinfo_bit_align);
      } else {
         mpm->testing.OUTBUF[vec->pt_size - 1] &= 0xFF << (8 - vec->auxinfo_bit_align);
      }
   }


   // compare data
   y = 0;
   if (dir == DIR_ENC) {
      if (vec->icvpos == IP_ICV_OFFSET) {
         if (memcmp (mpm->testing.OUTBUF, vec->ct, vec->ct_size - vec->icv_size)) {
            printk(KERN_DEBUG "MPM: mpm_diag:: Comparing CT failed[%d]\n", __LINE__);
            if(vec->fail_mode == 0) {
               outdiff (mpm->testing.OUTBUF, vec->ct, vec->ct_size - vec->icv_size);
            }
            printk(KERN_DEBUG "MPM: mpm_diag:: fail_mode == %d\n", vec->fail_mode);
            y = 1;
         }
#if 0
         if (mpm->testing.OUTBUF[vec->ct_size - vec->icv_size] != 0) {
            printk(KERN_DEBUG "MPM: mpm_diag:: Boundary test fail[%d]\n", __LINE__);
            y = 1;
         }
#endif
         if (memcmp (mpm->testing.OUTBUF + icvoff, vec->icv, vec->icv_size)) {
            printk(KERN_DEBUG "MPM: mpm_diag:: Comparing ICV failed[%d]\n", __LINE__);
            if(vec->fail_mode == 0) {
               outdiff (mpm->testing.OUTBUF + icvoff, vec->icv, vec->icv_size);
            }
            printk(KERN_DEBUG "MPM: mpm_diag:: fail_mode == %d\n", vec->fail_mode);
            y = 1;
         }
      } else { // IP_ICV_APPEND
         if (memcmp (mpm->testing.OUTBUF, vec->ct, vec->ct_size)) {
            printk(KERN_DEBUG "MPM: mpm_diag:: Comparing CT failed[%d]\n", __LINE__);
            if(vec->fail_mode == 0) {
               outdiff (mpm->testing.OUTBUF, vec->ct, vec->ct_size);
            }
            printk(KERN_DEBUG "MPM: mpm_diag:: fail_mode == %d\n", vec->fail_mode);
            y = 1;
         }
      }
   } else {
      if (memcmp (mpm->testing.OUTBUF, vec->pt, vec->pt_size)) {
         printk(KERN_DEBUG "MPM: mpm_diag:: Comparing PT failed[%d]\n", __LINE__);
         if(vec->fail_mode == 0){
            outdiff (mpm->testing.OUTBUF, vec->pt, vec->pt_size);
         }
         printk(KERN_DEBUG "MPM: mpm_diag:: fail_mode == %d\n", vec->fail_mode);
         y = 1;
      }
   }
ERR:
  if (y || mpmdiag_verbose) {
     printk(KERN_DEBUG "MPM: %s Vector %s-%s %s\n", dir == DIR_ENC ? "ENCRYPT" : "DECRYPT", mode_names[0][vec->enc_mode], mode_names[1][vec->hash_mode], (y == 0) ? "[PASSED]" : "[FAILED]");
  }


  if (key > 0) {
     mpm_free_key(mpm, key);
  }

  if (sgents_src) {
     DMA_UNMAP_SG(NULL, sgsrc, sgents_src, DMA_BIDIRECTIONAL);
     kfree(sgsrc);
     pdu_ddt_free(&src_ddt);
  }

  if (sgents_dst) {
     DMA_UNMAP_SG(NULL, sgdst, sgents_dst, DMA_BIDIRECTIONAL);
     kfree(sgdst);
     pdu_ddt_free(&dst_ddt);
  }
  free_test_vector(vec);
  mpm_free_spacc_ctx(mpm, 1, ctx);

  return y;
}

static ssize_t
store_mpmdiag(struct class *class, struct class_attribute *classattr,
                                  const char *buf, size_t count)
{
   char *c, test_vector_name[count+1];
   int err;

   strcpy(test_vector_name, buf);

   /*
    * Check for trailing newline and remove it.  We do not
    * support filenames containing newlines.
    */
   c = strchr(test_vector_name, '\n');
   if (c) {
      if (c[1] != '\0')
         return -EINVAL;
      *c = '\0';
   }

   err = mpm_vector(test_vector_name, DIR_DEC);
   return (err == 0) ? count : -EINVAL;
}

static struct class_attribute attrs[] = {
   __ATTR(vector, 0200, NULL, store_mpmdiag),
   __ATTR_NULL
};

static struct class mpmdiag_class = {
   .class_attrs = attrs,
   .owner = THIS_MODULE,
   .name = "mpmdiag",
};

static int __init mpmdiag_mod_init (void)
{
   return class_register(&mpmdiag_class);
}

static void __exit mpmdiag_mod_exit (void)
{
   mpm_device *mpm;

   class_unregister(&mpmdiag_class);
   mpm = mpm_get_device();
   if (mpm->testing.OUTBUF) { kfree(mpm->testing.OUTBUF); }
   if (mpm->testing.INBUF)  { kfree(mpm->testing.INBUF); }
   mpm->testing.OUTBUF = NULL;
   mpm->testing.INBUF  = NULL;
}

MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("Elliptic Technologies Inc.");
module_init (mpmdiag_mod_init);
module_exit (mpmdiag_mod_exit);

module_param(mpmdiag_verbose, int, 0);
MODULE_PARM_DESC(mpmdiag_verbose, "Enable verbose vector parsing (disabled by default)");

