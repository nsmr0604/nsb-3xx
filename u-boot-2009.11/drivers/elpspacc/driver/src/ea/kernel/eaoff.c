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
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <asm/param.h>
#include <linux/platform_device.h>
#include <linux/skbuff.h>
#include "elpea.h"
#include "elpeahw.h"

#define MAX_LEN 16384

struct eaoff_data {
   void *data;
   dma_addr_t data_dma;
};

static int eaoff_open(struct inode *inode, struct file *file)
{
   struct eaoff_data *priv;

   priv = kmalloc(sizeof *priv, GFP_KERNEL);
   if (!priv) {
      return -ENOMEM;
   }

   priv->data = dma_alloc_coherent(NULL, MAX_LEN, &priv->data_dma, GFP_KERNEL);
   if (!priv->data) {
      kfree(priv);
      return -ENOMEM;
   }

   file->private_data = priv;
   return 0;
}

static int eaoff_release(struct inode *inode, struct file *file)
{
   struct eaoff_data *priv = file->private_data;

   dma_free_coherent(NULL, MAX_LEN, priv->data, priv->data_dma);
   kfree(priv);
   return 0;
}

static ssize_t
eaoff_read(struct file *file, char __user *buf, size_t len, loff_t *off)
{
   return -EIO;
}


static void eaoff_write_cb(void *ea, void *_data,
                           uint32_t payload_len, uint32_t ret_code,
                           uint32_t soft_ttl,    uint32_t sw_id)
{
   struct semaphore *done_jobs = _data;

   if (ret_code != EA_RET_OK && ret_code != EA_RET_SEQ_ROLL) {
      pr_warn("EA FAILED: %u\n", (unsigned) ret_code);
   }
   up(done_jobs);
}

static uint32_t
eaoff_ctrl(unsigned cipher, unsigned ciphermode, unsigned cipherksize,
           unsigned hash,   unsigned hashmode,   unsigned hashksize)
{
   unsigned cipher_id, cipher_keylen, hash_id;

   switch (cipher) {
   case C_NULL:
      cipher_keylen = 0;
      cipher_id = 0;
      break;
   case C_DES:
      if (ciphermode != CM_CBC)
         return -1;
      cipher_id = CIPH_ALG_DES_CBC;

      if (cipherksize == 8)
         cipher_keylen = CKEY_LEN_128;
      else if (cipherksize == 24)
         cipher_keylen = CKEY_LEN_192;
      else
         return -1;
      break;
   case C_AES:
      switch (ciphermode) {
      case CM_CBC:
         cipher_id = CIPH_ALG_AES_CBC;
         break;
      case CM_CTR:
         cipher_id = CIPH_ALG_AES_CTR;
         break;
      case CM_CCM:
         if (hash || hashmode)
            return -1;
         cipher_id = CIPH_ALG_AES_CCM;
         break;
      case CM_GCM:
         if (hash || hashmode)
            return -1;
         cipher_id = CIPH_ALG_AES_GCM;
         break;
      default:
         return -1;
      }

      if (cipherksize == 16)
         cipher_keylen = CKEY_LEN_128;
      else if (cipherksize == 24)
         cipher_keylen = CKEY_LEN_192;
      else if (cipherksize == 32)
         cipher_keylen = CKEY_LEN_256;
      else
         return -1;

      break;
   default:
      return -1;
   }

   switch (hash) {
   case H_NULL:
      hash_id = 0;
      break;
   case H_MD5:
      if (hashmode != HM_HMAC)
         return -1;
      hash_id = MAC_ALG_HMAC_MD5_96;
      break;
   case H_SHA1:
      if (hashmode != HM_HMAC)
         return -1;
      hash_id = MAC_ALG_HMAC_SHA1_96;
      break;
   case H_SHA256:
      if (hashmode != HM_HMAC)
         return -1;
      hash_id = MAC_ALG_HMAC_SHA256_128;
      break;
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
   default:
      return -1;
   }

   return SA_CTRL_CIPH_ALG(cipher_id) | SA_CTRL_CKEY_LEN(cipher_keylen)
          | SA_CTRL_MAC_ALG(hash_id);
}

static ssize_t
eaoff_write(struct file *file, const char __user *buf, size_t len, loff_t *off)
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

   ea_sa sa = {0};

#ifdef USE_ZYNQ_OCM
   void *ocm_ba;
#endif

   struct eaoff_data *priv = file->private_data;
   unsigned datasize;
   ssize_t ret;
   int handle;
   int rc, queued, done;
   unsigned char cmd[16];



   ea_device *ea;

   ea = ea_get_device();

#ifdef EA_PROF
   memset(&(ea->stats), 0, sizeof ea->stats);
#endif

   rc = copy_from_user(cmd, buf, 16);
   if (rc != 0)
      return -EFAULT;
   ret = 16;
   runs         = (buf[10]<<16)|(buf[11]<<8)|buf[12];

   datasize = (cmd[6]<<8) | cmd[7];
   if (datasize > MAX_LEN) {
      return -EOVERFLOW;
   }

   sa.ctrl = eaoff_ctrl(cmd[0], cmd[1], cmd[2],
                        cmd[3], cmd[4], cmd[5]);
   if (sa.ctrl == -1) {
      return -ENXIO;
   }


#ifndef USE_ZYNQ_OCM
   srch = pdu_dma_alloc(sizeof *srch, &srch_dma);
   dsth = pdu_dma_alloc(128, &dsth_dma);
   if (!srch || !dsth) {
      ret = -ENOMEM;
      goto out_free_hdrs;
   }
#else
   // use data from the first 64K of OCM [assumed mapped high] to run the EA
   // in this demo we use the first 8K for SA data...
   ocm_ba   = ioremap_nocache(0xFFFC0000, 0x10000);
   srch_dma = 0xFFFC2000;
   srch     = ocm_ba + 0x2000;
   dsth_dma = 0xFFFC6000;
   dsth     = ocm_ba + 0x6000;
#endif

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

   srch->v4 = (struct iphdr) {
      .version = 4,
      .ihl = 5,
      .tot_len = htons(datasize),
   };
#ifndef USE_ZYNQ_OCM
   pdu_ddt_add(&src, srch_dma, sizeof srch->v4);
   pdu_ddt_add(&src, priv->data_dma, datasize);
   pdu_ddt_add(&dst, dsth_dma, 128);
   pdu_ddt_add(&dst, priv->data_dma, datasize);
#else
   pdu_ddt_add(&src, srch_dma, sizeof(srch->v4) + datasize);
   pdu_ddt_add(&dst, dsth_dma, 128 + datasize);
#endif

   handle = ea_open(ea, 1);
   if (handle < 0) {
      ret = pdu_error_code(handle);
      goto out_free_ddts;
   }

   sa.ctrl |= SA_CTRL_ACTIVE;
   rc = ea_build_sa(ea, handle, &sa);
   if (rc < 0) {
      ret = -ENODEV;
      goto out_free_handle;
   }

   sema_init(&done_jobs, 0);
   for (queued = done = 0; queued < runs;) {
      rc = ea_go(ea, handle, 1, &src, &dst, eaoff_write_cb, &done_jobs);
      if (rc >= 0) {
         queued++;
      } else if (rc == EA_ERROR) {
         WARN(1, "fatal processing error\n");
         ret = -EIO;
         break;
      } else {
         /* Wait for a FIFO slot to be available. */
//         printk("FIFO FULL\n");
         rc = down_interruptible(&done_jobs);
         if (rc < 0) {
            ret = rc;
            printk("eaoff:  User aborted: %d %d\n", queued, done);
            goto out_free_handle;
         }
         done++;
      }
   }

   /* Wait for pending jobs to finish */
   while (queued > done) {
      if (down_interruptible(&done_jobs)) {
         printk("eaoff:  User aborted: %d %d, %d\n", queued, done, EA_FIFO_STAT_STAT_CNT(pdu_io_read32(ea->regmap + EA_FIFO_STAT)));
         break;
      }
      done++;
   }

#ifdef EA_PROF
   {
     int x;
     printk("\nEA CMD IRQs: %d\n", ea->stats.cmd);
     for (x = 1; x <= 128; x++) {
        if (ea->stats.dc[x]) {
           printk("EA STAT LEVEL %d IRQs: == %zu\n", x, ea->stats.dc[x]);
        }
     }
  }
#endif


out_free_handle:
   ea_close(ea, handle);
out_free_ddts:
   pdu_ddt_free(&dst);
out_free_src:
   pdu_ddt_free(&src);
out_free_hdrs:
#ifndef USE_ZYNQ_OCM
   if (dsth)
      pdu_dma_free(128, dsth, dsth_dma);
   if (srch)
      pdu_dma_free(sizeof *srch, srch, srch_dma);
#else
   iounmap(ocm_ba);
#endif

   return ret;
}

static const struct file_operations eaoff_fops = {
   .owner   = THIS_MODULE,
   .open    = eaoff_open,
   .release = eaoff_release,
   .read    = eaoff_read,
   .write   = eaoff_write,
};

static struct miscdevice eaoff_device = {
   .minor = MISC_DYNAMIC_MINOR,
   .name = "spaccea",
   .fops = &eaoff_fops,
};

static int __init eaoff_mod_init (void)
{
   return misc_register(&eaoff_device);
}

static void __exit eaoff_mod_exit (void)
{
   misc_deregister(&eaoff_device);
}

MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("Elliptic Technologies Inc.");
module_init (eaoff_mod_init);
module_exit (eaoff_mod_exit);
