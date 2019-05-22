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
#include <asm/uaccess.h>
#include <asm/param.h>
#include <linux/platform_device.h>
#include "elpre.h"
#include "elprehw.h"
#include "elpspaccdrv.h"

static re_device re;

re_device *re_get_device(void)
{
   return &re;
}

void re_pop_jobs(unsigned long foo)
{
//   printk("RE tasklet\n");
   re_packet_dequeue(&re, -1);
}

static DECLARE_TASKLET(re_job_tasklet, re_pop_jobs, 0UL);

/* a function to run callbacks in the IRQ handler */
static irqreturn_t re_irq_handler(int irq, void *dev)
{
  uint32_t irq_stat, d;

  irq_stat = pdu_io_read32(re.regmap + RE_IRQ_STAT);
//printk("RE IRQ %08zx\n", irq_stat);
  d = 0;

  if (irq_stat & RE_IRQ_STAT_STAT) {
     d = 1;
     re.fifo_cnt = re.config.fifo_depth-1;
     pdu_io_write32(re.regmap+RE_IRQ_EN, RE_IRQ_EN_CMD|RE_IRQ_EN_STAT|RE_IRQ_EN_GLBL);
  }

  if (irq_stat & RE_IRQ_STAT_CMD) {
     d = 1;
     re.fifo_cnt = 1;
     pdu_io_write32(re.regmap+RE_IRQ_EN, RE_IRQ_EN_STAT|RE_IRQ_EN_GLBL);
  }

  if (d) {
     pdu_io_write32(re.regmap + RE_IRQ_STAT, irq_stat);
     pdu_io_write32(re.regmap+RE_IRQ_CTRL, RE_REG_IRQ_CTRL_STAT(re.fifo_cnt));
     tasklet_schedule(&re_job_tasklet);
  }

  return d ? IRQ_HANDLED : IRQ_NONE;
}


static int spaccre_probe(struct platform_device *pdev)
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
   printk("spaccre_probe: Device at %08lx(%08lx) of size %lu bytes\n", (unsigned long)res->start, (unsigned long)res->end, (unsigned long)resource_size(res));

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
   printk ("spaccre_probe::RE attaching to SPAcc EPN %X\n", ((pdu_info *)(pdev->dev.platform_data))->spacc_version.project);
   spacc = get_spacc_device_by_epn(((pdu_info *)(pdev->dev.platform_data))->spacc_version.project, 0);
   if (spacc == NULL) {
      return -ENODEV;
   }

   err = re_init(baseaddr, spacc, &re);
   if (err != CRYPTO_OK) {
      return -1;
   }

   // after we call re_init we need to allocate a pool
   re.sa_pool_virt = pdu_dma_alloc(re.sa_pool_size, &re.sa_pool_phys);
   if (!re.sa_pool_virt) {
      printk("spaccre_probe::Cannot allocate SA pool\n");
      re_fini(&re);
      return -1;
   }

   if (devm_request_irq(&pdev->dev, irq->start, re_irq_handler, IRQF_SHARED, dev_name(&pdev->dev), &pdev->dev)) {
      dev_err(&pdev->dev, "failed to request IRQ\n");
      re_fini(&re);
      pdu_dma_free(re.sa_pool_size, re.sa_pool_virt, re.sa_pool_phys);
      return -EBUSY;
   }

   // enable interrupts
   pdu_io_write32(baseaddr+RE_IRQ_CTRL, RE_REG_IRQ_CTRL_STAT(1));
   pdu_io_write32(baseaddr+RE_IRQ_EN, RE_IRQ_EN_STAT|RE_IRQ_EN_GLBL);

   return 0;
}

static int spaccre_remove(struct platform_device *pdev)
{
   printk("re_mod_exit::Freeing resources\n");

   // fini the SDK then free the SA pool
   pdu_io_write32(re.regmap+RE_IRQ_EN, 0); // disable IRQs
   re_fini(&re);
   pdu_dma_free(re.sa_pool_size, re.sa_pool_virt, re.sa_pool_phys);

   return 0;
}

static struct platform_driver spaccre_driver = {
   .probe  = spaccre_probe,
   .remove = spaccre_remove,
   .driver = {
      .name  = "spacc-re",
      .owner = THIS_MODULE
   }
};

static int __init re_mod_init (void)
{
   memset(&re, 0, sizeof re);
   return platform_driver_register(&spaccre_driver);
}

static void __exit re_mod_exit (void)
{
   platform_driver_unregister(&spaccre_driver);
}

MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("Elliptic Technologies Inc.");
module_init (re_mod_init);
module_exit (re_mod_exit);

// kernel
EXPORT_SYMBOL (re_get_device);

// lib functions
EXPORT_SYMBOL (re_reset_sa);
EXPORT_SYMBOL (re_set_next_read);
EXPORT_SYMBOL (re_set_next_write);
EXPORT_SYMBOL (re_start_operation);
EXPORT_SYMBOL (re_start_operation_ex);
EXPORT_SYMBOL (re_finish_operation);
EXPORT_SYMBOL (re_error_msg);
EXPORT_SYMBOL (re_retrieve_sa);
EXPORT_SYMBOL (re_write_sa);
EXPORT_SYMBOL (re_get_spacc_context);
EXPORT_SYMBOL (re_init_context_ex);
EXPORT_SYMBOL (re_get_context_ex);
EXPORT_SYMBOL (re_init_context);
EXPORT_SYMBOL (re_get_context);
EXPORT_SYMBOL (re_release_context);
EXPORT_SYMBOL (re_set_next_read_iv);
EXPORT_SYMBOL (re_set_next_read_key);
EXPORT_SYMBOL (re_set_next_read_mackey);
EXPORT_SYMBOL (re_set_next_read_params);
EXPORT_SYMBOL (re_set_next_read_sequence_number);
EXPORT_SYMBOL (re_set_next_write_iv);
EXPORT_SYMBOL (re_set_next_write_key);
EXPORT_SYMBOL (re_set_next_write_mackey);
EXPORT_SYMBOL (re_set_next_write_params);
EXPORT_SYMBOL (re_set_next_write_sequence_number);
EXPORT_SYMBOL (re_packet_dequeue);
EXPORT_SYMBOL (re_print_diag);

