#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pci.h>

#include "elpsaspa_hw.h"
#include "elppci.h"

static bool do_pci_reset = true;
module_param_named(pci_reset, do_pci_reset, bool, 0);

static bool do_pci_rings = true;
module_param_named(pci_rings, do_pci_rings, bool, 0);

static void saspa_pci_set_rings(struct pci_dev *pdev, int set)
{
   u32 cfg;

   pci_read_config_dword(pdev, ELPPCI_CTRL_REG, &cfg);
   if (set)
      cfg |= ELPPCI_TRNG_RINGS_EN_BIT;
   else
      cfg &= ~ELPPCI_TRNG_RINGS_EN_BIT;
   pci_write_config_dword(pdev, ELPPCI_CTRL_REG, cfg);
}

static void saspa_pci_set_dpa(struct pci_dev *pdev, int set)
{
   u32 cfg;

   pci_read_config_dword(pdev, ELPPCI_CTRL_REG, &cfg);
   if (set)
      cfg |= ELPPCI_DPA_EN_BIT;
   else
      cfg &= ~ELPPCI_DPA_EN_BIT;
   pci_write_config_dword(pdev, ELPPCI_CTRL_REG, cfg);
}

static void saspa_pci_enable_irqs(struct pci_dev *pdev)
{
   u32 cfg;

   pci_read_config_dword(pdev, ELPPCI_CTRL_REG, &cfg);
   pci_write_config_dword(pdev, ELPPCI_CTRL_REG, cfg | ELPPCI_IRQ_EN_BIT);
}

static void saspa_pci_reset(struct pci_dev *pdev)
{
   u32 cfg;

   pci_read_config_dword(pdev, ELPPCI_CTRL_REG, &cfg);
   pci_write_config_dword(pdev, ELPPCI_CTRL_REG, cfg | ELPPCI_RST_BIT);
   pci_write_config_dword(pdev, ELPPCI_CTRL_REG, cfg & ~ELPPCI_RST_BIT);
}

static int __devinit
saspa_pci_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
   struct platform_device *saspa;
   struct resource saspa_res[2];
   u32 __iomem *glbl_regs, tmp;
   int rc;

   rc = pci_enable_device(pdev);
   if (rc < 0) {
      dev_err(&pdev->dev, "failed to enable device\n");
      return rc;
   }

   saspa_res[0] = (struct resource) {
      .parent = &pdev->resource[1],
      .start = pci_resource_start(pdev, 1),
      .end   = pci_resource_start(pdev, 1) + SASPA_MEMORY_BASE - 1,
      .flags = IORESOURCE_MEM,
   };
   saspa_res[1] = (struct resource) {
      .start = pdev->irq,
      .end   = pdev->irq,
      .flags = IORESOURCE_IRQ,
   };

   /*
    * We must size the mem resource appropriately for available SASPA memory.
    * In our lab environment, this amount will alway match the configured
    * address width, which is available in the SASPA memory map.
    */
   glbl_regs = ioremap_nocache(saspa_res[0].start + SASPA_GLOBAL_BASE,
                               SASPA_GLOBAL_SIZE);
   if (!glbl_regs) {
      dev_err(&pdev->dev, "failed to map SASPA registers\n");
      return -ENOMEM;
   }

   tmp = readl(&glbl_regs[SASPA_GLBL_MAX_MEM_SZ]);
   tmp >>= SASPA_MAX_MEM_SZ;
   tmp &= (1ul << SASPA_MAX_MEM_SZ_BITS)-1;

   if (tmp > 0) {
      saspa_res[0].end += 16 << tmp;
   }

   iounmap(glbl_regs);

   if (do_pci_reset) {
      saspa_pci_reset(pdev);
   }

   saspa_pci_set_rings(pdev, do_pci_rings);
   saspa_pci_enable_irqs(pdev);

   if (resource_size(&saspa_res[0]) > pci_resource_len(pdev, 1)) {
      dev_err(&pdev->dev, "SASPA memory map does not fit in PCI BAR\n");
      return -ENODEV;
   }

   saspa = platform_device_register_resndata(&pdev->dev, "saspa", -1,
                                             saspa_res, ARRAY_SIZE(saspa_res),
                                             NULL, 0);
   if (IS_ERR(saspa))
      return PTR_ERR(saspa);

   pci_set_drvdata(pdev, saspa);
   return 0;
}

static void __devexit saspa_pci_remove(struct pci_dev *pdev)
{
   struct platform_device *saspa = pci_get_drvdata(pdev);

   platform_device_unregister(saspa);
   pci_disable_device(pdev);
}

static DEFINE_PCI_DEVICE_TABLE(saspa_pci_ids) = {
   { PCI_DEVICE(0xe117, 0x5a5f) },
   { 0 },
};
MODULE_DEVICE_TABLE(pci, saspa_pci_ids);

static struct pci_driver saspa_pci_driver = {
   .name     = "saspa-pci",
   .probe    = saspa_pci_probe,
   .remove   = saspa_pci_remove,
   .id_table = saspa_pci_ids,
};

/*
 * XXX: Linux 3.3 has a handy macro to eliminate this boilerplate:
 * module_driver(saspa_pci_driver, pci_register_driver, pci_unregister_driver);
 */
static int __init saspa_pci_init(void)
{
   return pci_register_driver(&saspa_pci_driver);
}
module_init(saspa_pci_init);

static void __exit saspa_pci_exit(void)
{
   pci_unregister_driver(&saspa_pci_driver);
}
module_exit(saspa_pci_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Elliptic Technologies Inc.");
MODULE_DESCRIPTION("Lab PCI driver for SASPA.");
