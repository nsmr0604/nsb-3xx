#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

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
#include <linux/ratelimit.h>
#include <linux/io.h>

#include <linux/fs.h>
#include <linux/dma-mapping.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <asm/param.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>

#include "eape.h"

#define DEVNAME "eape: "

static eape_device eape;

eape_device *eape_get_device(void)
{
   return &eape;
}

void eape_pop_jobs(unsigned long _eape)
{
   eape_pop_packet(&eape);
}

static DECLARE_TASKLET(eape_job_tasklet, eape_pop_jobs, 0UL);

/* a function to run callbacks in the IRQ handler */
static irqreturn_t eape_irq_handler(int irq, void *dev)
{
   uint32_t irq_stat;

   irq_stat = pdu_io_read32(eape.regmap + EAPE_REG_IRQ_STAT);
   if (irq_stat) {
      pdu_io_write32(eape.regmap + EAPE_REG_IRQ_STAT, irq_stat);
      tasklet_schedule(&eape_job_tasklet);
      return IRQ_HANDLED;
   }

   return IRQ_NONE;
}

static int eape_probe(struct platform_device *pdev)
{
   void *baseaddr;
   struct resource *res, *irq;
   int err;

   irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
   res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
   if (!res || !irq) {
      return -EINVAL;
   }
   printk("%s eape_probe: Device at %08lx(%08lx) of size %lu bytes\n", DEVNAME, (unsigned long)res->start, (unsigned long)res->end, (unsigned long)resource_size(res));

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

  /* init the library */
   err = eape_init(&eape, baseaddr, 32);
   if (err) {
      return -1;
   }

   // after we call re_init we need to allocate a pool
   printk("%s requires %zu bytes for SA\n", DEVNAME, eape.sa_ptr_mem_req);
   eape.sa_ptr_virt = pdu_dma_alloc(eape.sa_ptr_mem_req, &eape.sa_ptr_phys);
   if (!eape.sa_ptr_virt) {
      printk("Cannot allocate SA pool\n");
      return -1;
   }

   if (devm_request_irq(&pdev->dev, irq->start, eape_irq_handler, IRQF_SHARED, dev_name(&pdev->dev), &pdev->dev)) {
      dev_err(&pdev->dev, "failed to request IRQ\n");
      pdu_dma_free(eape.sa_ptr_mem_req, eape.sa_ptr_virt, eape.sa_ptr_phys);
      return -EBUSY;
   }

   // enable interrupts
   pdu_io_write32(baseaddr+EAPE_REG_IRQ_EN,EAPE_IRQ_EN_OUT_CMD_EN|EAPE_IRQ_EN_OUT_STAT_EN|EAPE_IRQ_EN_IN_CMD_EN|EAPE_IRQ_EN_IN_STAT_EN|EAPE_IRQ_EN_GLBL_EN);

   return 0;
}

static int eape_remove(struct platform_device *pdev)
{
   printk("%s eape_mod_exit::Freeing resources\n", DEVNAME);

   // fini the SDK then free the SA pool
   eape_deinit(&eape);
   pdu_dma_free(eape.sa_ptr_mem_req, eape.sa_ptr_virt, eape.sa_ptr_phys);

   return 0;
}

static struct platform_driver eape_driver = {
   .probe  = eape_probe,
   .remove = eape_remove,
   .driver = {
      .name  = "eape",
      .owner = THIS_MODULE
   }
};

static int __init eape_mod_init (void)
{
   memset(&eape, 0, sizeof eape);
   return platform_driver_register(&eape_driver);
}

static void __exit eape_mod_exit (void)
{
   platform_driver_unregister(&eape_driver);
}

MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("Elliptic Technologies Inc.");
module_init (eape_mod_init);
module_exit (eape_mod_exit);

// kernel
EXPORT_SYMBOL (eape_get_device);

// lib functions
EXPORT_SYMBOL (eape_init);
EXPORT_SYMBOL (eape_deinit);
EXPORT_SYMBOL (eape_open);
EXPORT_SYMBOL (eape_build_sa);
EXPORT_SYMBOL (eape_go);
EXPORT_SYMBOL (eape_close);
EXPORT_SYMBOL (eape_done);
EXPORT_SYMBOL (eape_pop_packet);


