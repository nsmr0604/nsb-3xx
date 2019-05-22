/*
 * RNG driver for Ellipsys RNGs
 *
 * Copyright 2012 (c)
 *
 * with the majority of the code coming from:
 *
 * Hardware driver for a True Random Number Generators (TRNG)
 * (c) Copyright 2012 Ellipsys Technologies <aelias@elliptictech.com>
 *
 *
 * This file is licensed under  the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <asm/uaccess.h>
#include <asm/param.h>
#include <linux/err.h>
#include <linux/hw_random.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>

#ifdef ELPTRNG_CRYPTO_API
#include <linux/crypto.h>
#include <crypto/internal/rng.h>
#endif

#ifdef ARMELPVEX1
#include <mach/motherboard.h>
#endif

#include "crypto/elpcrypto_api.h"
#include "elptrng_driver.h"
#include "elptrnghw.h"

#ifdef TRNGDIAG_ENABLE
#include "trngdiag.h"
#endif

static int elptrng_platform_driver_read(struct platform_device *pdev, void *buf, size_t max, bool wait);
static int elptrng_platform_driver_seed(struct platform_device *pdev, void *seed, size_t max, int bnonce, int wait);

#ifdef ELPTRNG_CRYPTO_API

typedef struct
{
   struct crypto_alg alg;
   void *dev;
} elliptic_crypto_driver;

static void elptrng_crypto_show(struct seq_file *m, struct crypto_alg *alg)
{
   seq_printf(m, "type         : %s\n", ELPCRYPTO_TRNG_TYPE);
   seq_printf(m, "seedsize     : %u\n", alg->cra_rng.seedsize);
}

static unsigned int elptrng_crypto_ctxsize(struct crypto_alg *alg, unsigned int type, unsigned int mask)
{
   return alg->cra_ctxsize;
}

static int elptrng_crypto_init_ops(struct crypto_tfm *tfm, unsigned int type, unsigned int mask)
{
   struct rng_alg *alg = &tfm->__crt_alg->cra_rng;
   struct rng_tfm *ops = &tfm->crt_rng;

   ops->rng_gen_random = alg->rng_make_random;
   ops->rng_reset = alg->rng_reset;

   return 0;
}

const struct crypto_type elliptic_crypto_trng_type = {
   .ctxsize = elptrng_crypto_ctxsize,
   .init = elptrng_crypto_init_ops,
   .show = elptrng_crypto_show,
};

#endif


#ifdef ELPTRNG_DEBUG_API

static ssize_t
show_read(struct device *dev, struct device_attribute *devattr, char *buf)
{
   char rand_val[TRNG_DATA_SIZE_BYTES];
   int i;

#if (defined ELPTRNG_CRYPTO_API)
   struct crypto_rng *rng;
   int err;

   rng = crypto_alloc_rng(ELPCRYPTO_TRNG_DRIVER, 0, 0);
   err = PTR_ERR(rng);
   if (IS_ERR(rng)) {
      ELPHW_PRINT("Error retrieving RNG type\n");
      return 0;
   }

   crypto_rng_get_bytes(rng, rand_val, TRNG_DATA_SIZE_BYTES);

   crypto_free_rng(rng);
#elif (defined ELPTRNG_HWRANDOM)
   elliptic_trng_driver* data = (elliptic_trng_driver *)platform_get_drvdata(to_platform_device(dev));
   elptrng_hwrng_driver_read((struct hwrng *)data->hwrng_drv, rand_val, TRNG_DATA_SIZE_BYTES, 0);
#else
   elptrng_platform_driver_read(to_platform_device(dev), rand_val, TRNG_DATA_SIZE_BYTES, 0);
#endif

   for(i=0; i<TRNG_DATA_SIZE_BYTES; i++) {
      ELPHW_PRINT("%.2x ", (rand_val[i] & 0xff));
   }
   ELPHW_PRINT("\n");

   return 0;
}

static DEVICE_ATTR(read, 0400, show_read, 0);

static const struct attribute_group trng_attr_group = {
   .attrs = (struct attribute *[]){
      &dev_attr_read.attr,
      NULL
   },
};

#endif

#ifdef ELPTRNG_CRYPTO_API
int elptrng_crypto_driver_read(struct crypto_rng *tfm, u8 *rdata, unsigned int dlen)
{
   elliptic_crypto_driver *data = 0;
   struct platform_device *pdev = 0;

   if (tfm == 0) {
      return -1;
   }

   data = (elliptic_crypto_driver *)container_of(crypto_rng_tfm(tfm)->__crt_alg, elliptic_crypto_driver, alg);
   pdev = (struct platform_device *)data->dev;

   /*ELPHW_PRINT("elptrng_crypto_driver_read (device name [%s])\n", pdev->name);*/

   return elptrng_platform_driver_read(pdev, rdata, dlen, 0);
}

int elptrng_crypto_driver_reset(struct crypto_rng *tfm, u8 *seed, unsigned int slen)
{
   elliptic_crypto_driver *data = 0;
   struct platform_device *pdev = 0;

   if (tfm == 0) {
      return -1;
   }

   data = (elliptic_crypto_driver *)container_of(crypto_rng_tfm(tfm)->__crt_alg, elliptic_crypto_driver, alg);
   pdev = (struct platform_device *)data->dev;

   /*ELPHW_PRINT("elptrng_crypto_driver_reset\n");*/

   return elptrng_platform_driver_seed(pdev, (unsigned char *)seed, (size_t)slen, ELLIPTIC_TRNG_CRYPTO_RESEED_NONCE_SUPPORT, ELPTRNG_WAIT);
}
#endif


#ifdef ELPTRNG_HWRANDOM
int elptrng_hwrng_driver_read(struct hwrng *rng, void *buf, size_t max, bool wait)
{
   struct platform_device *pdev = 0;

   if (rng == 0) {
      return -1;
   }

   pdev = (struct platform_device *)rng->priv;
   return elptrng_platform_driver_read(pdev, buf, max, wait);
}
#endif


static
int elptrng_platform_driver_read(struct platform_device *pdev, void *buf, size_t max, bool wait)
{
   elliptic_trng_driver *data = 0;
   int trng_error = -1;

   if ((pdev == 0) ||
       (buf == 0) ||
       (max == 0) ) {
      return trng_error;
   }

   data = platform_get_drvdata(pdev);

   if (data == 0) {
      return trng_error;
   }

   if (max > TRNG_DATA_SIZE_BYTES) {
      max = TRNG_DATA_SIZE_BYTES;
   }

   trng_error = -trng_rand (&data->trng, (unsigned char *)buf, max, 0);

   if (trng_error != ELPTRNG_OK) {
      ELPHW_PRINT("Error generating random TRNG value [%d]\n", trng_error);
   }

   return max;
}

static int elptrng_platform_driver_seed(struct platform_device *pdev, void *seed, size_t max, int bnonce, int wait)
{
   elliptic_trng_driver *data = 0;
   int trng_error = -1;
   int err = 0;
   int i;
   uint32_t *seed32;

   if ((pdev == 0) ||
       (seed == 0) ||
       (max == 0) ) {
      return trng_error;
   }

   data = platform_get_drvdata(pdev);

   if (bnonce == 1) {
      if (max > TRNG_NONCE_SIZE_BYTES) {
        /* why did they send somethin bigger??????? */
        ELPHW_PRINT("Truncating seed size from %d to %d\n", max, TRNG_NONCE_SIZE_BYTES);
        max = TRNG_NONCE_SIZE_BYTES;
      } else if (max < TRNG_NONCE_SIZE_BYTES) {
         /* why did they send somethin smaller - oh well, do the HW reseed */
         ELPHW_PRINT("Seed size to small %d. Doing HW reseed.\n", max);
         max = TRNG_NONCE_SIZE_BYTES;
         bnonce = 0;
      }
   }

   /* check for zeros */
   if (bnonce == 1) {
      int add = 0;

      seed32 = (uint32_t *)seed;
      for (i=0; i<(TRNG_NONCE_SIZE_WORDS/2); i++) {
         add |= seed32[i];
      }

      if (add == 0) {
         bnonce = 0;
      } else {
         add = 0;
         for (i=0; i<(TRNG_NONCE_SIZE_WORDS/2); i++) {
            add |= seed32[(TRNG_NONCE_SIZE_WORDS/2) + i];
         }

         if (add == 0) {
            bnonce = 0;
         }
      }
   }

   if (bnonce == 1) {
      trng_error = trng_reseed_nonce(&data->trng, seed, 0);
   } else {
      trng_error = trng_reseed_random(&data->trng, wait, 0);
   }

   if (trng_error != ELPTRNG_OK) {
      ELPHW_PRINT("Error generating random TRNG value [%d]\n", trng_error);
      err = -1;
   }

   return err;
}

/* a function to run callbacks in the IRQ handler */
static trng_hw *trng=NULL;
static struct completion trng_complete;

trng_hw *trng_get_device(void)
{
   return trng;
}

static irqreturn_t trng_irq_handler(int irq, void *dev)
{
  uint32_t irq_stat, d;

  irq_stat = pdu_io_read32(trng->trng_irq_stat);

  d = 0;
  if (irq_stat & TRNG_IRQ_DONE) {
     d = 1;
     //complete(&trng_complete);
     pdu_io_write32(trng->trng_irq_stat, TRNG_IRQ_DONE);
  }

  return d ? IRQ_HANDLED : IRQ_NONE;
}

void trng_user_init_wait(trng_hw *hw)
{
   init_completion(&trng_complete);
}

void trng_user_wait(trng_hw *hw)
{
   wait_for_completion(&trng_complete);
}

static int elptrng_driver_probe(struct platform_device *pdev)
{
   struct resource *cfg, *irq;
   elliptic_trng_driver *data;
   int ret;
#ifdef ELPTRNG_HWRANDOM
   struct hwrng *hwrng_driver_info = 0;
#endif

#ifdef ELPTRNG_CRYPTO_API
   elliptic_crypto_driver* crypto_driver_info = 0;
#endif

   cfg = platform_get_resource(pdev, IORESOURCE_MEM, 0);
   irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);

   if (!cfg || !irq) {
      ELPHW_PRINT("no memory or IRQ resource\n");
      return -ENOMEM;
   }

   printk("trng_probe: Device at %08lx(%08lx) of size %lu bytes\n", (unsigned long)cfg->start, (unsigned long)cfg->end, (unsigned long)resource_size(cfg));

   if (!devm_request_mem_region(&pdev->dev, cfg->start, resource_size(cfg), ELLIPTIC_TRNG_DRIVER_PLATFORM_NAME "-cfg")) {
      ELPHW_PRINT("unable to request io mem\n");
      return -EBUSY;
   }

   data = devm_kzalloc(&pdev->dev, sizeof(elliptic_trng_driver), GFP_KERNEL);
   if (!data) {
      return -ENOMEM;
   }

   platform_set_drvdata(pdev, data);

#ifdef PCI_INDIRECT
   data->base_addr = cfg->start;
#else
   data->base_addr = (unsigned int *)devm_ioremap_nocache(&pdev->dev, cfg->start, resource_size(cfg));
#endif
   if (!data->base_addr) {
      devm_kfree(&pdev->dev, data);
      ELPHW_PRINT("unable to remap io mem\n");
      return -ENOMEM;
   }

   trng = &data->trng;

#if 1
   if (devm_request_irq(&pdev->dev, irq->start, trng_irq_handler, IRQF_SHARED, dev_name(&pdev->dev), &pdev->dev)) {
      dev_err(&pdev->dev, "failed to request IRQ\n");
      return -EBUSY;
   }
#endif

   if ( (ret = -trng_init(&data->trng, (unsigned int)data->base_addr, ELPTRNG_IRQ_PIN_DISABLE, ELPTRNG_RESEED)) != ELPTRNG_OK) {
      ELPHW_PRINT("TRNG init failed (%d)\n", ret);
      devm_kfree(&pdev->dev, data);
      return ret;
   }

#ifdef ELPTRNG_HWRANDOM
   hwrng_driver_info = devm_kzalloc(&pdev->dev, sizeof(struct hwrng), GFP_KERNEL);

   if (!hwrng_driver_info) {
      devm_kfree(&pdev->dev, data);
      return -ENOMEM;
   }

   hwrng_driver_info->name = devm_kzalloc(&pdev->dev, sizeof(ELLIPTIC_HWRNG_DRIVER_NAME) + 1, GFP_KERNEL);

   if (!hwrng_driver_info->name) {
      devm_kfree(&pdev->dev, data);
      devm_kfree(&pdev->dev, hwrng_driver_info);
      return -ENOMEM;
   }

   memset((void *)hwrng_driver_info->name, 0, sizeof(ELLIPTIC_HWRNG_DRIVER_NAME) + 1);
   strcpy((char *)hwrng_driver_info->name, ELLIPTIC_HWRNG_DRIVER_NAME);

   hwrng_driver_info->read = &elptrng_hwrng_driver_read;
   hwrng_driver_info->data_present = 0;
   hwrng_driver_info->priv = (unsigned long)pdev;

   data->hwrng_drv = hwrng_driver_info;

   ret = hwrng_register(hwrng_driver_info);

   if (ret) {
      ELPHW_PRINT("unable to load HWRNG driver (error %d)\n", ret);
      devm_kfree(&pdev->dev, (void *)hwrng_driver_info->name);
      devm_kfree(&pdev->dev, hwrng_driver_info);
      devm_kfree(&pdev->dev, data);
      return ret;
   }

   ELPHW_PRINT("ELP TRNG registering HW_RANDOM\n");
#endif

#ifdef ELPTRNG_CRYPTO_API

   crypto_driver_info = devm_kzalloc(&pdev->dev, sizeof(elliptic_crypto_driver), GFP_KERNEL);

   strcpy(crypto_driver_info->alg.cra_name, ELPCRYPTO_TRNG_NAME);
   strcpy(crypto_driver_info->alg.cra_driver_name, ELPCRYPTO_TRNG_DRIVER);
   crypto_driver_info->alg.cra_priority = ELPCRYPTO_TRNG_PRIORITY;
   crypto_driver_info->alg.cra_flags = CRYPTO_ALG_TYPE_RNG;
   crypto_driver_info->alg.cra_ctxsize = 0;
   crypto_driver_info->alg.cra_type = &elliptic_crypto_trng_type;
   crypto_driver_info->alg.cra_module = THIS_MODULE;
   /*crypto_driver_info->alg.cra_list = LIST_HEAD_INIT(crypto_driver_info->alg.cra_list);*/
   crypto_driver_info->alg.cra_u.rng.rng_make_random = &elptrng_crypto_driver_read;
   crypto_driver_info->alg.cra_u.rng.rng_reset = &elptrng_crypto_driver_reset;
   crypto_driver_info->alg.cra_u.rng.seedsize = TRNG_NONCE_SIZE_BYTES;

   crypto_driver_info->dev = pdev;

   data->crypto_drv = crypto_driver_info;

   ret = crypto_register_alg(&crypto_driver_info->alg);

   if (ret) {
      ELPHW_PRINT("unable to load CRYPTO API driver (error %d)\n", ret);
#ifdef ELPTRNG_HWRANDOM
      hwrng_unregister(hwrng_driver_info);
      devm_kfree(&pdev->dev, (void *)hwrng_driver_info->name);
      devm_kfree(&pdev->dev, hwrng_driver_info);
#endif
      devm_kfree(&pdev->dev, crypto_driver_info);
      devm_kfree(&pdev->dev, data);
      return ret;
   }

   ELPHW_PRINT("ELP TRNG registering CRYPTO API\n");
#endif


#ifdef ELPTRNG_DEBUG_API
   return sysfs_create_group(&pdev->dev.kobj, &trng_attr_group);
#else
   return 0;
#endif

}

static int elptrng_driver_remove(struct platform_device *pdev)
{
   elliptic_trng_driver *data = platform_get_drvdata(pdev);

#ifdef ELPTRNG_HWRANDOM
   struct hwrng *hwrng_driver_info = (struct hwrng *)data->hwrng_drv;
#endif

#ifdef ELPTRNG_CRYPTO_API
   elliptic_crypto_driver* crypto_driver_info = (elliptic_crypto_driver *)data->crypto_drv;
#endif

   ELPHW_PRINT("ELP TRNG removed\n");

   trng_close(&data->trng);

#ifdef ELPTRNG_DEBUG_API
   sysfs_remove_group(&pdev->dev.kobj, &trng_attr_group);
#endif

#ifdef ELPTRNG_HWRANDOM
   ELPHW_PRINT("ELP TRNG unregistering from HW_RANDOM\n");
   hwrng_unregister(hwrng_driver_info);
   devm_kfree(&pdev->dev, (void *)hwrng_driver_info->name);
   devm_kfree(&pdev->dev, hwrng_driver_info);
#endif

#ifdef ELPTRNG_CRYPTO_API
   ELPHW_PRINT("ELP TRNG unregistering from CRYPTO API\n");
   crypto_unregister_alg(&crypto_driver_info->alg);
   devm_kfree(&pdev->dev, crypto_driver_info);
#endif

   devm_kfree(&pdev->dev, data);

   return 0;
}

static struct platform_driver s_elptrng_platform_driver_info = {
   .probe      = elptrng_driver_probe,
   .remove     = elptrng_driver_remove,
   .driver     = {
      .name = ELLIPTIC_TRNG_DRIVER_PLATFORM_NAME,
      .owner   = THIS_MODULE,
   },
};

static int __init elptrng_platform_driver_start(void)
{
   ELPHW_PRINT("elptrng_platform_driver_start\n");
   return platform_driver_register(&s_elptrng_platform_driver_info);
}

static void __exit elptrng_platform_driver_end(void)
{
   ELPHW_PRINT("elptrng_platform_driver_end\n");
   platform_driver_unregister(&s_elptrng_platform_driver_info);
}

module_init(elptrng_platform_driver_start);
module_exit(elptrng_platform_driver_end);

MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("Elliptic Technologies Inc.");

#ifdef ELPTRNG_CRYPTO_API
EXPORT_SYMBOL(elptrng_crypto_driver_read);
EXPORT_SYMBOL(elptrng_crypto_driver_reset);
#endif

#ifdef ELPTRNG_HWRANDOM
EXPORT_SYMBOL(elptrng_hwrng_driver_read);
#endif

EXPORT_SYMBOL(trng_user_init_wait);
EXPORT_SYMBOL(trng_user_wait);

EXPORT_SYMBOL(trng_reseed_nonce);
EXPORT_SYMBOL(trng_rand);
EXPORT_SYMBOL(trng_get_device);


