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

#include <linux/io.h>

#include <linux/fs.h>
#include <asm/uaccess.h>
#include <asm/param.h>
#include <linux/platform_device.h>
#include "elpkep.h"
#include "elpspaccdrv.h"

static kep_device kep;

kep_device *kep_get_device(void)
{
   return &kep;
}

/* a function to run callbacks in the IRQ handler */
static irqreturn_t kep_irq_handler(int irq, void *dev)
{
  uint32_t irq_stat, d;
    
  irq_stat = pdu_io_read32(kep.regmap + KEP_REG_IRQ_STAT);
   
  d = 0;

  if (irq_stat & KEP_IRQ_STAT_STAT) {
     d = 1;
     // clear any pending jobs
     kep_done(&kep, -1);
     pdu_io_write32(kep.regmap + KEP_REG_IRQ_STAT, KEP_IRQ_STAT_STAT);
  }

  return d ? IRQ_HANDLED : IRQ_NONE;
}


static int spacckep_probe(struct platform_device *pdev)
{
   void *baseaddr;
   struct resource *res, *irq;
   spacc_device *spacc;

   int err;
   
   irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
   res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
   if (!res || !irq) {
      return -EINVAL;
   }
   
   printk("spacckep_probe: Device at %08lx(%08lx) of size %lu bytes\n", (unsigned long)res->start, (unsigned long)res->end, (unsigned long)resource_size(res));
#ifdef PCI_INDIRECT
   baseaddr = res->start;
   printk("Using indirect mode\n");
#else
   baseaddr = devm_ioremap_nocache(&pdev->dev, res->start, resource_size(res));
#endif

   if (!baseaddr) {
      dev_err(&pdev->dev, "unable to map iomem\n");
      return -ENXIO;
   }

   /* get the associated spacc */
   printk ("KEP attaching to SPAcc EPN %X\n", ((pdu_info *)(pdev->dev.platform_data))->spacc_version.project);
   spacc = get_spacc_device_by_epn(((pdu_info *)(pdev->dev.platform_data))->spacc_version.project, 0);
   if (spacc == NULL) {
      return -ENODEV;
   }

   err = kep_init(baseaddr, spacc, &kep);
   if (err != CRYPTO_OK) {
      return -1;
   }

   if (devm_request_irq(&pdev->dev, irq->start, kep_irq_handler, IRQF_SHARED, dev_name(&pdev->dev), &pdev->dev)) {
      dev_err(&pdev->dev, "failed to request IRQ\n");
      return -EBUSY;
   }

   // enable interrupts
   pdu_io_write32(baseaddr+KEP_REG_IRQ_CTRL, KEP_REG_IRQ_CTRL_STAT(1));
   pdu_io_write32(baseaddr+KEP_REG_IRQ_EN, KEP_IRQ_EN_STAT|KEP_IRQ_EN_GLBL);
     
   return 0;
}

static int spacckep_remove(struct platform_device *pdev)
{
   printk("kep_mod_exit::Freeing resources\n");

   kep_fini(&kep);
   return 0;   
}

static struct platform_driver spacckep_driver = {
   .probe  = spacckep_probe,
   .remove = spacckep_remove,
   .driver = {
      .name  = "spacc-kep",
      .owner = THIS_MODULE
   }
};   

static int __init kep_mod_init (void)
{
   memset(&kep, 0, sizeof kep);
   return platform_driver_register(&spacckep_driver);
}

static void __exit kep_mod_exit (void)
{
   platform_driver_unregister(&spacckep_driver);
}


MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("Elliptic Technologies Inc.");
module_init (kep_mod_init);
module_exit (kep_mod_exit);

// kernel
EXPORT_SYMBOL (kep_get_device);

// lib
EXPORT_SYMBOL (kep_open);
EXPORT_SYMBOL (kep_is_valid);
EXPORT_SYMBOL (kep_load_keys);
EXPORT_SYMBOL (kep_go);
EXPORT_SYMBOL (kep_done);
EXPORT_SYMBOL (kep_close);

