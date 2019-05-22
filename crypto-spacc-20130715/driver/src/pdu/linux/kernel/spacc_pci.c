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
#include <linux/kernel.h>       /* printk() */
#include <linux/slab.h>         /* kmalloc() */
#include <linux/fs.h>           /* everything... */
#include <linux/errno.h>        /* error codes */
#include <linux/types.h>        /* size_t */

#include <linux/fcntl.h>        /* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <linux/delay.h>        /* udelay */
#include <linux/sched.h>
#include <linux/kdev_t.h>
#include <linux/mm.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/poll.h>
#include <linux/wait.h>         /* wait queue */
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/platform_device.h>

#include <linux/dma-mapping.h>
#include <linux/vmalloc.h>

//#include <asm/system.h>         /* cli(), *_flags, mb() */
#include <asm/uaccess.h>        /* copy_*_user */
#include <asm/io.h>             /* memcpy_fromio */

#include "elppdu.h"
#include "elppci.h"

void pdu_pci_set_dpa_off(struct pci_dev *tif)
{
   unsigned int conf = 0;
   pci_read_config_dword( tif, ELPPCI_CTRL_REG,&conf);
   pci_write_config_dword( tif, ELPPCI_CTRL_REG, conf & ~ELPPCI_DPA_EN_BIT);
}

void pdu_pci_set_dpa_on(struct pci_dev *tif)
{
   unsigned int conf = 0;
   pci_read_config_dword( tif, ELPPCI_CTRL_REG,&conf);
   pci_write_config_dword( tif, ELPPCI_CTRL_REG, conf | ELPPCI_DPA_EN_BIT);
}


void pdu_pci_set_trng_ring_off(struct pci_dev *tif)
{
   unsigned int conf = 0;
   pci_read_config_dword( tif, ELPPCI_CTRL_REG,&conf);
   pci_write_config_dword( tif, ELPPCI_CTRL_REG, conf & ~ELPPCI_TRNG_RINGS_EN_BIT);
}

void pdu_pci_set_trng_ring_on(struct pci_dev *tif)
{
   unsigned int conf = 0;
   pci_read_config_dword( tif, ELPPCI_CTRL_REG,&conf);
   pci_write_config_dword( tif, ELPPCI_CTRL_REG, conf | ELPPCI_TRNG_RINGS_EN_BIT);
}

void pdu_pci_set_little_endian(struct pci_dev *tif)
{
   unsigned int conf = 0;
   pci_read_config_dword( tif, ELPPCI_CTRL_REG,&conf);
   pci_write_config_dword( tif, ELPPCI_CTRL_REG, conf | ELPPCI_LITTLE_ENDIAN_BIT );
}

void pdu_pci_set_big_endian(struct pci_dev *tif)
{
   unsigned int conf = 0;
   pci_read_config_dword( tif, ELPPCI_CTRL_REG,&conf);
   pci_write_config_dword( tif, ELPPCI_CTRL_REG,conf & ~(ELPPCI_LITTLE_ENDIAN_BIT));
}

void pdu_pci_set_secure_off(struct pci_dev *tif)
{
   unsigned int conf = 0;
   pci_read_config_dword( tif, ELPPCI_CTRL_REG,&conf);
   pci_write_config_dword( tif, ELPPCI_CTRL_REG, conf & ~ELPPCI_SECURE_BIT);
}

void pdu_pci_set_secure_on(struct pci_dev *tif)
{
   unsigned int conf = 0;
   pci_read_config_dword( tif, ELPPCI_CTRL_REG,&conf);
   pci_write_config_dword( tif, ELPPCI_CTRL_REG, conf | ELPPCI_SECURE_BIT);
}

void pdu_pci_interrupt_enabled(struct pci_dev *tif)
{
   unsigned int conf = 0;
   pci_read_config_dword( tif, ELPPCI_CTRL_REG, &conf);
   pci_write_config_dword( tif, ELPPCI_CTRL_REG, conf | ELPPCI_IRQ_EN_BIT );
}

void pdu_pci_reset(struct pci_dev *tif)
{
   unsigned int conf = 0;
   pci_read_config_dword( tif, ELPPCI_CTRL_REG, &conf);
   pci_write_config_dword( tif, ELPPCI_CTRL_REG, conf | ELPPCI_RST_BIT);
   pci_read_config_dword( tif, ELPPCI_CTRL_REG, &conf);
   pci_write_config_dword( tif, ELPPCI_CTRL_REG, conf & ~(ELPPCI_RST_BIT));
}

void pdu_pci_or(struct pci_dev *tif, unsigned val)
{
   unsigned int conf;
   pci_read_config_dword( tif, ELPPCI_CTRL_REG, &conf);
   pci_write_config_dword( tif, ELPPCI_CTRL_REG, conf | val);
}


int pci_reset, pci_rings, dpa_on;
unsigned PCI_DID = PDU_PCI_DID;
int pci_or;

// max of 16 devices
#define MAX_DEV 16

/* Each device gets an MMIO and (sometimes) an IRQ resource. */
static struct resource resources[MAX_DEV][2];
static struct platform_device devices[MAX_DEV];
static int dev_id = 0;
static pdu_info *info;

#ifdef PCI_INDIRECT
void *pci_indirect_bus;
void elppci_set_ptr(void *);
#endif

void pdu_pci_release_dev (struct device *dev){
   printk("PDU device '%s' has been released.\n", dev_name(dev));
}

static int pdu_pci_probe (void)
{
   int x;
   void *pdu_mem;
   unsigned long baseaddr;

   struct pci_dev *dev;
   int rc;

   dev = pci_get_device (0xE117, PCI_DID, NULL);
   if (dev == NULL) {
      printk ("pdu_probe: Device not found\n");
      return -ENODEV;
   }

   rc = pci_enable_device(dev);
   if (rc < 0) {
      put_device(&dev->dev);
      printk ("pdu_probe: Failed to enable device\n");
      return rc;
   }

#ifdef PCI_INDIRECT
   baseaddr = pci_resource_start(dev, 0);
   pci_indirect_bus = ioremap_nocache(baseaddr, 0x1000);
   elppci_set_ptr(pci_indirect_bus);
#endif

   // find base address from BAR1
   baseaddr = pci_resource_start(dev, 1);

   // perform PCI reset if requested
   if (pci_reset) {
      pdu_pci_reset (dev);
   }

   // ENABLE TRNG rings, or turn off by default
   if (pci_rings) {
      pdu_pci_set_trng_ring_on (dev);
   } else {
      pdu_pci_set_trng_ring_off (dev);
   }

   if (dpa_on) {
      pdu_pci_set_dpa_on(dev);
   } else {
      pdu_pci_set_dpa_off(dev);
   }

#if defined(SDK_ENDIAN_LITTLE)
   pdu_pci_set_little_endian (dev);
#elif defined(SDK_ENDIAN_BIG)
   pdu_pci_set_big_endian (dev);
#endif

   /*
    * Enable PCI interrupt routing.  Interrupts still need to be enabled
    * individually for each device.
    */
   pdu_pci_interrupt_enabled (dev);

#ifdef SECURE_MODE
   if (secure_mode) {
      pdu_pci_set_secure_on (dev);
   } else {
      pdu_pci_set_secure_off (dev);
   }
#endif

   pdu_pci_or(dev, pci_or);

   memset (&resources, 0, sizeof resources);
   memset (&devices, 0, sizeof devices);

#if !defined(PDU_SINGLE_CORE) || defined(PDU_SINGLE_EAPE)
   if (pdu_mem_init()) {
      printk("pdu_probe::Failed to initialize DMA pool\n");
      return -1;
   }
#endif

#ifndef PDU_SINGLE_CORE

   info = kzalloc(sizeof *info, GFP_KERNEL);
   if (unlikely(!info))
      return -ENOMEM;

   // at this point 'baseaddr' == base of device1, we map 4K of it to read the PDU config
#ifdef PCI_INDIRECT
   pdu_mem = baseaddr;
#else
   pdu_mem = ioremap_nocache (baseaddr, 0x1000);
#endif

   // read the config to see what's up
   if (pdu_get_version (pdu_mem, info)) {
      printk("PDU::Failed to retrieve version from SPAcc core\n");
      return -1;
   }


   // Enable interrupts for the SPAcc-PDU.
   if (info->spacc_version.is_pdu) {
      pdu_io_write32 (pdu_mem, PDU_IRQ_EN_GLBL);
   }

   dev_id = 0;

  if (info->spacc_version.is_pdu || info->spacc_version.is_spacc) {
      // setup a base spacc or spacc device,

      for (x = 0; x < info->spacc_config.num_vspacc; x++) {
         resources[dev_id][0].start = baseaddr + (info->spacc_version.is_pdu ? 0x40000 : 0) + (0x40000 * x);
         resources[dev_id][0].end = resources[dev_id][0].start + 0x40000 - 1;
         resources[dev_id][0].flags = IORESOURCE_MEM;

         resources[dev_id][1].start = dev->irq;
         resources[dev_id][1].end = dev->irq;
         resources[dev_id][1].flags = IORESOURCE_IRQ;

         devices[dev_id].name = "spacc";
         devices[dev_id].id = (info->spacc_version.project << 16) | x;

         if (info->spacc_version.is_pdu) {
            pdu_io_write32(pdu_mem, pdu_io_read32(pdu_mem) | PDU_IRQ_EN_VSPACC(x));
         }

         ++dev_id;
      }
   } else if (info->spacc_version.is_hsm) {
      // shared - shared context so do like virtual 0 and 1 where 1 has to request contexts from 0
      // memory regions are offset in hw -- this is SoC specific
      for (x = 0; x < 2; x++) {
         resources[dev_id][0].start = baseaddr + (0x40000 * x);
         resources[dev_id][0].end = resources[dev_id][0].start + 0x40000 - 1;
         resources[dev_id][0].flags = IORESOURCE_MEM;

         resources[dev_id][1].start = dev->irq;
         resources[dev_id][1].end = dev->irq;
         resources[dev_id][1].flags = IORESOURCE_IRQ;

         devices[dev_id].name = "spacc";
         devices[dev_id].id = (info->spacc_version.project << 16) | x;
         if (info->spacc_version.is_pdu) {
            pdu_io_write32(pdu_mem, pdu_io_read32(pdu_mem) | PDU_IRQ_EN_VSPACC(x));
         }
         ++dev_id;
      }
   }

   if (info->spacc_version.is_pdu) {
      spdu_boot_trng(info, baseaddr);

      // RE
      if (info->pdu_config.is_re) {
         resources[dev_id][0].start = baseaddr + 0x8000;
         resources[dev_id][0].end = resources[dev_id][0].start + 0x4000 - 1;
         resources[dev_id][0].flags = IORESOURCE_MEM;

         resources[dev_id][1].start = dev->irq;
         resources[dev_id][1].end = dev->irq;
         resources[dev_id][1].flags = IORESOURCE_IRQ;

         devices[dev_id].name = "spacc-re";
         devices[dev_id].id = -1;
         pdu_io_write32(pdu_mem, pdu_io_read32(pdu_mem) | PDU_IRQ_EN_RE);
         ++dev_id;
      }
      // KEP
      if (info->pdu_config.is_kep) {
         resources[dev_id][0].start = baseaddr + 0x10000;
         resources[dev_id][0].end = resources[dev_id][0].start + 16384 - 1;
         resources[dev_id][0].flags = IORESOURCE_MEM;

         resources[dev_id][1].start = dev->irq;
         resources[dev_id][1].end = dev->irq;
         resources[dev_id][1].flags = IORESOURCE_IRQ;

         devices[dev_id].name = "spacc-kep";
         devices[dev_id].id = -1;
         pdu_io_write32(pdu_mem, pdu_io_read32(pdu_mem) | PDU_IRQ_EN_KEP);
         ++dev_id;
      }
      // TRNG
      if (info->pdu_config.is_rng) {
         resources[dev_id][0].start = baseaddr + 0x18000;
         resources[dev_id][0].end = resources[dev_id][0].start + 32768 - 1;
         resources[dev_id][0].flags = IORESOURCE_MEM;

         resources[dev_id][1].start = dev->irq;
         resources[dev_id][1].end = dev->irq;
         resources[dev_id][1].flags = IORESOURCE_IRQ;

         devices[dev_id].name = "trng";
         devices[dev_id].id = -1;
         //pdu_io_write32(pdu_mem, pdu_io_read32(pdu_mem) | PDU_IRQ_EN_RNG);
         ++dev_id;
      }
      // PKA
      if (info->pdu_config.is_pka) {
         resources[dev_id][0].start = baseaddr + 0x20000;
         resources[dev_id][0].end = resources[dev_id][0].start + 131072 - 1;
         resources[dev_id][0].flags = IORESOURCE_MEM;

         resources[dev_id][1].start = dev->irq;
         resources[dev_id][1].end = dev->irq;
         resources[dev_id][1].flags = IORESOURCE_IRQ;

         devices[dev_id].name = "pka";
         devices[dev_id].id = -1;
         pdu_io_write32(pdu_mem, pdu_io_read32(pdu_mem) | PDU_IRQ_EN_PKA);
         ++dev_id;
      }
      if (info->pdu_config.is_mpm) {
         resources[dev_id][0].start = baseaddr + 0xC000;
         resources[dev_id][0].end = resources[dev_id][0].start + (0xDFFF - 0xC000);
         resources[dev_id][0].flags = IORESOURCE_MEM;

         resources[dev_id][1].start = dev->irq;
         resources[dev_id][1].end = dev->irq;
         resources[dev_id][1].flags = IORESOURCE_IRQ;

         devices[dev_id].name = "spacc-mpm";
         devices[dev_id].id = 0;
         pdu_io_write32(pdu_mem, pdu_io_read32(pdu_mem) | PDU_IRQ_EN_MPM);
         ++dev_id;
      }
#ifdef PDU_DUAL_MPM
      // this configures a second parallel MPM with id "1".  Only enable on platforms
      // that support dual-MPM.
      if (info->pdu_config.is_mpm) {
         resources[dev_id][0].start = baseaddr + 0xE000;
         resources[dev_id][0].end = resources[dev_id][0].start + (0xFFFF - 0xE000);
         resources[dev_id][0].flags = IORESOURCE_MEM;

         resources[dev_id][1].start = dev->irq;
         resources[dev_id][1].end = dev->irq;
         resources[dev_id][1].flags = IORESOURCE_IRQ;

         devices[dev_id].name = "spacc-mpm";
         devices[dev_id].id = 1;
         pdu_io_write32(pdu_mem, pdu_io_read32(pdu_mem) | PDU_IRQ_EN_MPM1);
         ++dev_id;
      }
#endif

      if (info->pdu_config.is_ea) {
         resources[dev_id][0].start = baseaddr + 0x14000;
         resources[dev_id][0].end = resources[dev_id][0].start + (0x7FFF - 0x4000);
         resources[dev_id][0].flags = IORESOURCE_MEM;

         resources[dev_id][1].start = dev->irq;
         resources[dev_id][1].end = dev->irq;
         resources[dev_id][1].flags = IORESOURCE_IRQ;

         devices[dev_id].name = "spacc-ea";
         devices[dev_id].id = -1;
         pdu_io_write32(pdu_mem, pdu_io_read32(pdu_mem) | PDU_IRQ_EN_EA);
         ++dev_id;
      }
   }

   printk("PDU IRQ MAP: %08lx\n", pdu_io_read32(pdu_mem));

#ifndef PCI_INDIRECT
   // release the mapping
   iounmap (pdu_mem);
#endif

#else
/* single core */
#ifdef PDU_SINGLE_TRNG
         resources[dev_id][0].start = baseaddr;
         resources[dev_id][0].end = resources[dev_id][0].start + 32768 - 1;
         resources[dev_id][0].flags = IORESOURCE_MEM;

         resources[dev_id][1].start = dev->irq;
         resources[dev_id][1].end = dev->irq;
         resources[dev_id][1].flags = IORESOURCE_IRQ;

         devices[dev_id].name = "trng";
         devices[dev_id].id = -1;
         ++dev_id;
#endif

#ifdef PDU_SINGLE_EAPE
         resources[dev_id][0].start = baseaddr;
         resources[dev_id][0].end = resources[dev_id][0].start + 0x100 - 1;
         resources[dev_id][0].flags = IORESOURCE_MEM;

         resources[dev_id][1].start = dev->irq;
         resources[dev_id][1].end = dev->irq;
         resources[dev_id][1].flags = IORESOURCE_IRQ;

         devices[dev_id].name = "eape";
         devices[dev_id].id = -1;
         ++dev_id;
#endif

#ifdef PDU_SINGLE_PKA
         resources[dev_id][0].start = baseaddr;
         resources[dev_id][0].end = resources[dev_id][0].start + 131072 - 1;
         resources[dev_id][0].flags = IORESOURCE_MEM;

         resources[dev_id][1].start = dev->irq;
         resources[dev_id][1].end = dev->irq;
         resources[dev_id][1].flags = IORESOURCE_IRQ;

         devices[dev_id].name = "pka";
         devices[dev_id].id = -1;
         pdu_io_write32(pdu_mem, pdu_io_read32(pdu_mem) | PDU_IRQ_EN_PKA);
         ++dev_id;
#endif

#endif

   /* now register them all */
   for (x = 0; x < dev_id; x++) {
      devices[x].num_resources = ARRAY_SIZE(resources[x]);
      devices[x].resource= resources[x];
      devices[x].dev.release = &pdu_pci_release_dev;
      devices[x].dev.platform_data = info;
      platform_device_register (&devices[x]);
      printk ("pdu_probe: registered %s\n", dev_name(&devices[x].dev));
   }

   return 0;
}

static void pdu_pci_remove(void)
{
   int i;
   for (i = 0; i < dev_id; i++) {
      platform_device_unregister (&devices[i]);
   }
#ifdef PCI_INDIRECT
   iounmap(pci_indirect_bus);
#endif

#if !defined(PDU_SINGLE_CORE) || defined(PDU_SINGLE_EAPE)
   pdu_mem_deinit();
#endif
}

static int __init pdu_pci_mod_init(void)
{
   return pdu_pci_probe();
}

static void __exit pdu_pci_mod_exit(void)
{
   return pdu_pci_remove();
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Elliptic Technologies Inc.");
module_init(pdu_pci_mod_init);
module_exit(pdu_pci_mod_exit);

module_param(pci_or, int, 0);
MODULE_PARM_DESC(pci_or, "Value to OR into PCI config register");
module_param (pci_reset, int, 0);
MODULE_PARM_DESC (pci_reset, "Reset the PCI");
module_param (pci_rings, int, 0);
MODULE_PARM_DESC (pci_rings, "Set the PCI rings");
module_param (PCI_DID, int, 0);
MODULE_PARM_DESC (PCI_DID, "PCI DID to bind to");
module_param (dpa_on, int, 0);
MODULE_PARM_DESC (dpa_on, "Set the DPA flag");

EXPORT_SYMBOL (pdu_pci_set_trng_ring_off);
EXPORT_SYMBOL (pdu_pci_set_trng_ring_on);
EXPORT_SYMBOL (pdu_pci_set_little_endian);
EXPORT_SYMBOL (pdu_pci_set_big_endian);
EXPORT_SYMBOL (pdu_pci_set_secure_off);
EXPORT_SYMBOL (pdu_pci_set_secure_on);
EXPORT_SYMBOL (pdu_pci_interrupt_enabled);
EXPORT_SYMBOL (pdu_pci_reset);

