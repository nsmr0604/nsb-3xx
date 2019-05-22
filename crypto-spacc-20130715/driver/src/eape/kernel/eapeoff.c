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
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/io.h>

#include <linux/fs.h>
#include <linux/dma-mapping.h>
#include <linux/completion.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/ratelimit.h>
#include <asm/uaccess.h>
#include <asm/param.h>
#include <linux/platform_device.h>
#include <linux/skbuff.h>
#include "eape.h"

#define MAX_LEN 16384

struct eapeoff_data {
   eape_device *eape;

   void *data;
   dma_addr_t data_dma;
};

static int eapeoff_open(struct inode *inode, struct file *file)
{
   struct eapeoff_data *priv;

   priv = kmalloc(sizeof *priv, GFP_KERNEL);
   if (!priv) {
      return -ENOMEM;
   }

   priv->eape = eape_get_device();
   if (!priv->eape) {
      kfree(priv);
      return -ENODEV;
   }

   priv->data = dma_alloc_coherent(NULL, MAX_LEN, &priv->data_dma, GFP_KERNEL);
   if (!priv->data) {
      kfree(priv);
      return -ENOMEM;
   }

   file->private_data = priv;
   return 0;
}

static int eapeoff_release(struct inode *inode, struct file *file)
{
   struct eapeoff_data *priv = file->private_data;

   dma_free_coherent(NULL, MAX_LEN, priv->data, priv->data_dma);
   kfree(priv);
   return 0;
}

static ssize_t
eapeoff_read(struct file *file, char __user *buf, size_t len, loff_t *off)
{
   return -EIO;
}

static void eapeoff_write_cb(void *ea, void *_data,
                           uint32_t payload_len, uint32_t ret_code,
                           uint32_t sw_id)
{
   struct semaphore *done_jobs = _data;

   if (ret_code != EAPE_OK && ret_code != EAPE_SEQ_ROLL) {
      pr_warn("EAPE FAILED: %u\n", (unsigned) ret_code);
   }

   up(done_jobs);
}

enum ecipher
{
  C_NULL   = 0,
  C_DES    = 1,
  C_AES    = 2,
  C_RC4    = 3,
  C_MULTI2 = 4,
  C_KASUMI = 5,
  C_SNOW3G_UEA2 = 6,
  C_ZUC_UEA3 = 7,

  C_MAX
};

enum eciphermode
{
  CM_ECB = 0,
  CM_CBC = 1,
  CM_CTR = 2,
  CM_CCM = 3,
  CM_GCM = 5,
  CM_OFB = 7,
  CM_CFB = 8,
  CM_F8  = 9,
  CM_XTS = 10,

  CM_MAX
};

enum ehash
{
  H_NULL   = 0,
  H_MD5    = 1,
  H_SHA1   = 2,
  H_SHA224 = 3,
  H_SHA256 = 4,
  H_SHA384 = 5,
  H_SHA512 = 6,
  H_XCBC   = 7,
  H_CMAC   = 8,
  H_KF9    = 9,
  H_SNOW3G_UIA2 = 10,
  H_CRC32_I3E802_3 = 11,
  H_ZUC_UIA3 = 12,
  H_SHA512_224 = 13,
  H_SHA512_256 = 14,
  H_MAX
};

enum ehashmode
{
  HM_RAW    = 0,
  HM_SSLMAC = 1,
  HM_HMAC   = 2,

  HM_MAX
};


static uint32_t
eapeoff_ctrl(unsigned cipher, unsigned ciphermode, unsigned cipherksize,
           unsigned hash,   unsigned hashmode,   unsigned hashksize)
{
   unsigned cipher_id, cipher_keylen, hash_id, aesoff;

   aesoff = 0;
   switch (cipherksize) {
   case 24: aesoff = 1; break;
   case 32: aesoff = 2; break;
   }


   switch (cipher) {
   case C_NULL:
      cipher_keylen = 0;
      cipher_id = EAPE_SA_ALG_CIPH_NULL;
      break;
   case C_DES:
      if (ciphermode != CM_CBC)
         return -1;

      if (cipherksize == 8)
         cipher_id = EAPE_SA_ALG_CIPH_DES_CBC;
      else if (cipherksize == 24)
         cipher_id = EAPE_SA_ALG_CIPH_3DES_CBC;
      else
         return -1;
      break;
   case C_AES:
      switch (ciphermode) {
      case CM_CBC:
         cipher_id = EAPE_SA_ALG_CIPH_AES128_CBC + aesoff;
         break;
      case CM_CTR:
         cipher_id = EAPE_SA_ALG_CIPH_AES128_CTR + aesoff;
         break;
/*
      case CM_CCM:
         if (hash || hashmode)
            return -1;
         cipher_id = CIPH_ALG_AES_CCM;
         break;
*/
         case CM_GCM:
         if (hash || hashmode)
            return -1;
         cipher_id = EAPE_SA_ALG_CIPH_AES128_GCM + aesoff;
         break;
      default:
         return -1;
      }
      break;
   default:
      return -1;
   }

   switch (hash) {
   case H_NULL:
      hash_id = EAPE_SA_ALG_AUTH_NULL;
      break;
   case H_MD5:
      if (hashmode != HM_HMAC)
         return -1;
      hash_id = EAPE_SA_ALG_AUTH_MD5_96;
      break;
   case H_SHA1:
      if (hashmode != HM_HMAC)
         return -1;
      hash_id = EAPE_SA_ALG_AUTH_SHA1_96;
      break;
   case H_SHA256:
      if (hashmode != HM_HMAC)
         return -1;
      hash_id = EAPE_SA_ALG_AUTH_SHA256;
      break;
/*
   case H_SHA384:
      if (hashmode != HM_HMAC)
         return -1;
      hash_id = MAC_ALG_HMAC_SHA384_192;
      break;
   case H_SHA512:
      if (hashmode != HM_HMAC)
         return -1;
      hash_id = MAC_ALG_HMAC_SHA512_256;
      break;
   case H_XCBC:
      if (hashmode != HM_RAW)
         return -1;
      hash_id = MAC_ALG_AES_XCBC_MAC_96;
      break;
   case H_CMAC:
      if (hashmode != HM_RAW)
         return -1;
      hash_id = MAC_ALG_AES_CMAC_96;
      break;
*/
   default:
      return -1;
   }

   return cipher_id | hash_id;
}

static ssize_t
eapeoff_write(struct file *file, const char __user *buf, size_t len, loff_t *off)
{
   struct semaphore done_jobs;
   union {
      struct iphdr v4;
      struct ipv6hdr v6;
   } *srch;
   void *dsth;

   uint32_t runs;
   dma_addr_t srch_dma, dsth_dma;
   pdu_ddt src, dst;

   eape_sa sa = {0};

   struct eapeoff_data *priv = file->private_data;
   unsigned datasize;
   ssize_t ret;
   int handle;
   int rc, queued, done;
   unsigned char cmd[13];

   eape_device *eape;

   rc = copy_from_user(cmd, buf, 13);
   if (rc != 0)
      return -EFAULT;
   ret = 13;
   runs         = (buf[10]<<16)|(buf[11]<<8)|buf[12];


   datasize = (cmd[6]<<8) | cmd[7];
   if (datasize > MAX_LEN) {
      return -EOVERFLOW;
   }

   sa.alg = eapeoff_ctrl(cmd[0], cmd[1], cmd[2],
                        cmd[3], cmd[4], cmd[5]);
   if (sa.alg == -1) {
      return -ENXIO;
   }

   srch = dma_alloc_coherent(NULL, sizeof *srch, &srch_dma, GFP_KERNEL);
   dsth = dma_alloc_coherent(NULL, 128, &dsth_dma, GFP_KERNEL);
   if (!srch || !dsth) {
      ret = -ENOMEM;
      goto out_free_hdrs;
   }

   rc = pdu_ddt_init(&src, 2);
   if (rc < 0) {
      ret = pdu_error_code(rc);
      goto out_free_hdrs;
   }

   rc = pdu_ddt_init(&dst, 3);
   if (rc < 0) {
      ret = pdu_error_code(rc);
      goto out_free_src;
   }

   if (1) {
      srch->v4 = (struct iphdr) {
         .version = 4,
         .ihl = 5,
         .tot_len = htons(datasize),
      };
      pdu_ddt_add(&src, srch_dma, sizeof srch->v4);
   }
   pdu_ddt_add(&src, priv->data_dma, datasize);

   pdu_ddt_add(&dst, dsth_dma, 128);
   pdu_ddt_add(&dst, priv->data_dma, datasize);

   handle = eape_open(priv->eape);
   if (handle < 0) {
      ret = pdu_error_code(handle);
      goto out_free_ddts;
   }

   sa.flags |= EAPE_SA_FLAGS_ACTIVE;
   eape = priv->eape;
   rc = eape_build_sa(priv->eape, handle, &sa);
   if (rc < 0) {
      ret = -ENODEV;
      goto out_free_handle;
   }

   sema_init(&done_jobs, 0);
   for (queued = done = 0; queued < runs;) {
      rc = eape_go(priv->eape, handle, 1, &src, &dst, eapeoff_write_cb, &done_jobs);
      if (rc >= 0) {
         queued++;
      } else if (rc == EAPE_ERR) {
         WARN(1, "fatal processing error\n");
         ret = -EIO;
         break;
      } else {
         /* Wait for a FIFO slot to be available. */
         rc = down_interruptible(&done_jobs);
         if (rc < 0) {
            ret = rc;
            break;
         }
         done++;
      }
   }

   /* Wait for pending jobs to finish */
   while (queued > done) {
      down(&done_jobs);
      done++;
   }

out_free_handle:
   eape_close(priv->eape, handle);
out_free_ddts:
   pdu_ddt_free(&dst);
out_free_src:
   pdu_ddt_free(&src);
out_free_hdrs:
   if (dsth)
      dma_free_coherent(NULL, 128, dsth, dsth_dma);
   if (srch)
      dma_free_coherent(NULL, sizeof *srch, srch, srch_dma);

   return ret;
}

static const struct file_operations eapeoff_fops = {
   .owner   = THIS_MODULE,
   .open    = eapeoff_open,
   .release = eapeoff_release,
   .read    = eapeoff_read,
   .write   = eapeoff_write,
};

static struct miscdevice eapeoff_device = {
   .minor = MISC_DYNAMIC_MINOR,
   .name = "eape",
   .fops = &eapeoff_fops,
};

static int __init eapeoff_mod_init (void)
{
   return misc_register(&eapeoff_device);
}

static void __exit eapeoff_mod_exit (void)
{
   misc_deregister(&eapeoff_device);
}

MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("Elliptic Technologies Inc.");
module_init (eapeoff_mod_init);
module_exit (eapeoff_mod_exit);
