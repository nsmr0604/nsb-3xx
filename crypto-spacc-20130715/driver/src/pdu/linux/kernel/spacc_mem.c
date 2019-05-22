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
#include <linux/platform_device.h>
#include <linux/io-mapping.h>
#include <linux/err.h>

#include "elppdu.h"

#ifdef PDU_ZYNQ_RESET
#include <mach/slcr.h>

void pdu_zynq_reset(void)
{
   xslcr_write(0x240, 0xF);
   xslcr_write(0x240, 0x0);
}
#endif

static unsigned long vex_baseaddr = PDU_BASE_ADDR;
module_param_named(baseaddr, vex_baseaddr, ulong, 0);
MODULE_PARM_DESC(baseaddr, "Hardware base address (default " __stringify(PDU_BASE_ADDR) ")");

// max of 16 devices
#define MAX_DEV 16

static struct platform_device *devices[MAX_DEV];
static int dev_id;

static int spdu_init(unsigned long baseaddr, pdu_info *info)
{
   void *pdu_mem;

   pr_info("SPAcc-PDU base address: %.8lx\n", baseaddr);

   pdu_mem = ioremap_nocache(baseaddr, 0x1000);
   if (!pdu_mem)
      return -ENOMEM;

   /* Read the config to see what's up */
   pdu_get_version(pdu_mem, info);

   /* Enable SPAcc-PDU interrupts. */
   pdu_io_write32(pdu_mem, PDU_IRQ_EN_GLBL); // enable all ints!

   iounmap(pdu_mem);
   return 0;
}

static void register_device(const char *name, int id,
                            const struct resource *res, unsigned num,
                            pdu_info *info)
{
   char suffix[16] = "";

   if (dev_id >= MAX_DEV) {
      pr_err("Too many devices; increase MAX_DEV.\n");
      return;
   }

   devices[dev_id] = platform_device_register_resndata(NULL, name, id,
                                                       res, num,
                                                       info, sizeof *info);
   if (IS_ERR(devices[dev_id])) {
      if (id >= 0)
         snprintf(suffix, sizeof suffix, ".%d", id);
      pr_err("Failed to register %s%s\n", name, suffix);

      devices[dev_id] = NULL;
      return;
   }

   dev_id++;
}

static int __init pdu_vex_mod_init(void)
{
   struct resource res[2];
   pdu_info info;
   int i, rc;
   void *pdu_mem;

   if (pdu_mem_init()) {
      printk("pdu_probe::Failed to initialize DMA pool\n");
      return -1;
   }

#ifdef PDU_ZYNQ_RESET
   pdu_zynq_reset();
#endif

#ifndef PDU_SINGLE_CORE

   rc = spdu_init(vex_baseaddr, &info);
   if (rc < 0)
      return -1;

#ifdef PDU_CORTINA_G2_FORCE_3RD_VIRTUAL_INTERFACE_USAGE 
 /* G2 has three virtual interaces at offset 0x00000, 0x40000, 0x80000. 
  * The first two interfaces are designed for use by Tensilica cores and sends interrupts to them, 
  * the third is for use by the Dual ARM cores running Linux and sends interrupts to the ARM interrupt controller. 
  * Force this Linux driver to use 3rd Virtual Interface at offset 0x80000 for use by the ARM cores.
  */
   pdu_mem = ioremap_nocache(vex_baseaddr, 0x1000);
   if (!pdu_mem)
      return -ENOMEM;

      unsigned long offset = 0x80000;

      res[0] = (struct resource) {
         .start = vex_baseaddr + offset,
         .end   = vex_baseaddr + offset + 0x40000-1,
         .flags = IORESOURCE_MEM,
      };
      res[1] = (struct resource) {
         .start = PDU_BASE_IRQ,
         .end   = PDU_BASE_IRQ,
         .flags = IORESOURCE_IRQ,
      };

      if (info.spacc_version.is_pdu) {
         pdu_io_write32(pdu_mem, pdu_io_read32(pdu_mem) | PDU_IRQ_EN_VSPACC(i));
      }
      register_device("spacc", i | (info.spacc_version.project << 16), res, 2, &info);
#else /* CORTINA_G2_FORCE_3RD_VIRTUAL_INTERFACE_USAGE */
   pdu_mem = ioremap_nocache(vex_baseaddr, 0x1000);
   if (!pdu_mem)
      return -ENOMEM;
   for (i = 0; i < info.spacc_config.num_vspacc; i++) {
      unsigned long offset = i*0x40000;

      if (info.spacc_version.is_pdu)
         offset += 0x40000;

      res[0] = (struct resource) {
         .start = vex_baseaddr + offset,
         .end   = vex_baseaddr + offset + 0x40000-1,
         .flags = IORESOURCE_MEM,
      };
      res[1] = (struct resource) {
         .start = PDU_BASE_IRQ,
         .end   = PDU_BASE_IRQ,
         .flags = IORESOURCE_IRQ,
      };

      if (info.spacc_version.is_pdu) {
         pdu_io_write32(pdu_mem, pdu_io_read32(pdu_mem) | PDU_IRQ_EN_VSPACC(i));
      }
      register_device("spacc", i | (info.spacc_version.project << 16), res, 2, &info);
   }
#endif /* CORTINA_G2_FORCE_3RD_VIRTUAL_INTERFACE_USAGE */

   if (info.spacc_version.is_pdu) {
      spdu_boot_trng(&info, vex_baseaddr);
      if (info.pdu_config.is_re) {
         res[0] = (struct resource) {
            .start = vex_baseaddr + 0x8000,
            .end   = vex_baseaddr + 0x8000 + 0x4000-1,
            .flags = IORESOURCE_MEM,
         };
         res[1] = (struct resource) {
            .start = PDU_BASE_IRQ,
            .end   = PDU_BASE_IRQ,
            .flags = IORESOURCE_IRQ,
         };

         pdu_io_write32(pdu_mem, pdu_io_read32(pdu_mem) | PDU_IRQ_EN_RE);
         register_device("spacc-re", -1, res, 2, &info);
      }

      if (info.pdu_config.is_kep) {
         res[0] = (struct resource) {
            .start = vex_baseaddr + 0x10000,
            .end   = vex_baseaddr + 0x10000 + 0x4000-1,
            .flags = IORESOURCE_MEM,
         };
         res[1] = (struct resource) {
            .start = PDU_BASE_IRQ,
            .end   = PDU_BASE_IRQ,
            .flags = IORESOURCE_IRQ,
         };

         pdu_io_write32(pdu_mem, pdu_io_read32(pdu_mem) | PDU_IRQ_EN_KEP);
         register_device("spacc-kep", -1, res, 2, &info);
      }

      if (info.pdu_config.is_mpm) {
         res[0] = (struct resource) {
            .start = vex_baseaddr + 0xC000,
            .end   = vex_baseaddr + 0xC000 +  (0xDFFF - 0xC000),
            .flags = IORESOURCE_MEM,
         };
         res[1] = (struct resource) {
            .start = PDU_BASE_IRQ,
            .end   = PDU_BASE_IRQ,
            .flags = IORESOURCE_IRQ,
         };

         pdu_io_write32(pdu_mem, pdu_io_read32(pdu_mem) | PDU_IRQ_EN_MPM);
         register_device("spacc-mpm", 0, res, 2, &info);
      }
#ifdef PDU_DUAL_MPM
      // this configures a second parallel MPM with id "1".  Only enable on platforms
      // that support dual-MPM.
      if (info.pdu_config.is_mpm) {
         res[0] = (struct resource) {
            .start = vex_baseaddr + 0xE000,
            .end   = vex_baseaddr + 0xE000 +  (0xFFFF - 0xE000),
            .flags = IORESOURCE_MEM,
         };
         res[1] = (struct resource) {
            .start = PDU_BASE_IRQ,
            .end   = PDU_BASE_IRQ,
            .flags = IORESOURCE_IRQ,
         };

         pdu_io_write32(pdu_mem, pdu_io_read32(pdu_mem) | PDU_IRQ_EN_MPM1);
         register_device("spacc-mpm", 1, res, 2, &info);
      }
#endif

      if (info.pdu_config.is_ea) {
         res[0] = (struct resource) {
            .start = vex_baseaddr + 0x14000,
            .end   = vex_baseaddr + 0x14000 + (0x7FFF - 0x4000),
            .flags = IORESOURCE_MEM,
         };
         res[1] = (struct resource) {
            .start = PDU_BASE_IRQ,
            .end   = PDU_BASE_IRQ,
            .flags = IORESOURCE_IRQ,
         };

         pdu_io_write32(pdu_mem, pdu_io_read32(pdu_mem) | PDU_IRQ_EN_EA);
         register_device("spacc-ea", -1, res, 2, &info);
      }

      if (info.pdu_config.is_rng) {
         res[0] = (struct resource) {
            .start = vex_baseaddr + 0x18000,
            .end   = vex_baseaddr + 0x18000 + 32768-1,
            .flags = IORESOURCE_MEM,
         };
         res[1] = (struct resource) {
            .start = PDU_BASE_IRQ,
            .end   = PDU_BASE_IRQ,
            .flags = IORESOURCE_IRQ,
         };

         pdu_io_write32(pdu_mem, pdu_io_read32(pdu_mem) | PDU_IRQ_EN_RNG);
         register_device("trng", -1, res, 2, &info);
      }

      if (info.pdu_config.is_pka) {
         res[0] = (struct resource) {
            .start = vex_baseaddr + 0x20000,
            .end   = vex_baseaddr + 0x20000 + 131072-1,
            .flags = IORESOURCE_MEM,
         };
         res[1] = (struct resource) {
            .start = PDU_BASE_IRQ,
            .end   = PDU_BASE_IRQ,
            .flags = IORESOURCE_IRQ,
         };

         pdu_io_write32(pdu_mem, pdu_io_read32(pdu_mem) | PDU_IRQ_EN_PKA);
         register_device("pka", -1, res, 2, &info);
      }
   }
   iounmap(pdu_mem);
#else

#ifdef PDU_SINGLE_EAPE
   res[0] = (struct resource) {
      .start = vex_baseaddr,
      .end   = vex_baseaddr + 0x100-1,
      .flags = IORESOURCE_MEM,
   };
   res[1] = (struct resource) {
      .start = PDU_BASE_IRQ,
      .end   = PDU_BASE_IRQ,
      .flags = IORESOURCE_IRQ,
   };
   register_device("eape", -1, res, 2, &info);
#endif

#ifdef PDU_SINGLE_TRNG
   res[0] = (struct resource) {
      .start = vex_baseaddr,
      .end   = vex_baseaddr + 32768-1,
      .flags = IORESOURCE_MEM,
   };
   res[1] = (struct resource) {
      .start = PDU_BASE_IRQ,
      .end   = PDU_BASE_IRQ,
      .flags = IORESOURCE_IRQ,
   };
   register_device("trng", -1, res, 2, &info);
#endif

#ifdef PDU_SINGLE_TRNG3
   res[0] = (struct resource) {
      .start = vex_baseaddr,
      .end   = vex_baseaddr + 0x80-1,
      .flags = IORESOURCE_MEM,
   };
   res[1] = (struct resource) {
      .start = PDU_BASE_IRQ,
      .end   = PDU_BASE_IRQ,
      .flags = IORESOURCE_IRQ,
   };
   register_device("trng3", -1, res, 2, &info);
#endif

#ifdef PDU_SINGLE_PKA
   res[0] = (struct resource) {
      .start = vex_baseaddr,
      .end   = vex_baseaddr + 131072-1,
      .flags = IORESOURCE_MEM,
   };
   res[1] = (struct resource) {
      .start = PDU_BASE_IRQ,
      .end   = PDU_BASE_IRQ,
      .flags = IORESOURCE_IRQ,
   };
   register_device("pka", -1, res, 2, &info);
#endif
#endif
   return 0;
}
module_init(pdu_vex_mod_init);

static void __exit pdu_vex_mod_exit(void)
{
   int i;

   for (i = 0; i < MAX_DEV; i++) {
      platform_device_unregister(devices[i]);
   }
   pdu_mem_deinit();
}
module_exit(pdu_vex_mod_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Elliptic Technologies Inc.");
