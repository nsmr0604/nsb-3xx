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
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/list.h>
#include <linux/miscdevice.h>

#include "elppdu.h"
#include "elpspacc.h"
#include "elpspaccdrv.h"

static uint32_t oldtimer = 100000, timer = 100000; // 1ms @45MHz

static int no_latency=0;


/* Context Manager Test */

// loop through available ctx

// request new single context
// release one

// request new multiple context
// release multiple

int __devinit ctx_manager_test (spacc_device *spacc)
{
   int ret = 0;
   int ctx;
   int old_ctx;

   /* pass ctx_id = -1 to allocate a new one */
   ctx = spacc_ctx_request(spacc, -1, 1);
   if (ctx < 0) {
      printk("[FAILED] To allocate a handle\n");
      return ctx;
   }
   ret = spacc_ctx_release(spacc, ctx);
   if (ret != 0) {
      printk("[FAILED] To free a context\n");
      return ret;
   }
   printk ("[PASSED]:Simple ctx request/release\n");

   /* pass ctx_id = -1 to allocate a new one */
   /* new one should be the same as the just released one */
   old_ctx = ctx;
   ctx = spacc_ctx_request(spacc, -1, 1);
   if (ctx < 0) {
      return ctx;
   }
   if (ctx != old_ctx) {
      return -1;
   }
   ret = spacc_ctx_release(spacc, ctx);
   if (ret != 0) {
      return ret;
   }
   printk ("[PASSED]:Second simple ctx request/release\n");

   if (spacc->config.is_hsm_shared == 0) {
      /* pass ctx_id = -1 to allocate new ones */
      ctx = spacc_ctx_request(spacc, -1, 3);
      if (ctx < 0) {
         return ctx;
      }

      ret = spacc_ctx_release(spacc, ctx);
      if (ret != 0) {
         return ret;
      }
      printk ("[PASSED]:multiple ctx request/release\n");

      /* pass ctx_id = -1 to allocate new ones */
      ctx = spacc_ctx_request(spacc, -1, 3);
      if (ctx < 0) {
         return ctx;
      }
      /* reuse same ctx */
      ret = spacc_ctx_request(spacc, ctx, 3);
      if (ret != ctx) {
         return -1;
      }

      ret = spacc_ctx_release(spacc, ctx);
      if (ret != 0) {
         return ret;
      }
      ret = spacc_ctx_release(spacc, ctx);
      if (ret != 0) {
         return ret;
      }
      printk ("[PASSED]:multiple requests on same ctx /release\n");

      /* pass ctx_id = -1 and allocate all available  */
      printk ("num available [%d]\n", spacc->config.num_ctx);
      ctx = spacc_ctx_request(spacc, -1, spacc->config.num_ctx);
      if (ctx < 0) {
         return ctx;
      }
      old_ctx = ctx;

      /* request one more should fail gracefully */
      ctx = spacc_ctx_request(spacc, -1, 1);
      if (ctx != -1) {
         return -1;
      }
      printk ("[PASSED]:multiple requests when full /release\n");

      /* request three more should fail gracefully */
      ctx = spacc_ctx_request(spacc, -1, 3);
      if (ctx != -1) {
         return -1;
      }

      ret = spacc_ctx_release(spacc, old_ctx);
      if (ret != 0) {
         return ret;
      }
      printk ("[PASSED]:multiple requests when full /release\n");
   }

   return ret;
}

struct platform_device * get_spacc_platdev_by_epn(uint32_t epn, uint32_t virt)
{
   char name[256];
   struct device *dev;

   snprintf(name, sizeof(name), "spacc.%d", (epn << 16) | virt);
   dev = bus_find_device_by_name(&platform_bus_type, NULL, name);
   if (!dev) {
      printk(KERN_ERR "failed to find device for %s\n", name);
      return NULL;
   }
   return to_platform_device(dev);
}
EXPORT_SYMBOL(get_spacc_platdev_by_epn);

/* This is used by RE and KEP to get the spacc device */
spacc_device * get_spacc_device_by_epn(uint32_t epn, uint32_t virt)
{
   struct platform_device *plat;
   struct spacc_priv *priv;

   plat = get_spacc_platdev_by_epn(epn, virt);
   if (!plat)
      return NULL;

   priv = platform_get_drvdata(plat);
   return &priv->spacc;
}
EXPORT_SYMBOL(get_spacc_device_by_epn);

/* a function to run callbacks in the IRQ handler */
static irqreturn_t spacc_irq_handler(int irq, void *dev)
{
   struct spacc_priv *priv = platform_get_drvdata(to_platform_device(dev));
   spacc_device *spacc = &priv->spacc;

   if (oldtimer != timer) {
      spacc_set_wd_count(&priv->spacc, priv->spacc.config.wd_timer = timer);
      printk("spacc::Changed timer from %zu to %zu\n", oldtimer, timer);
      oldtimer = timer;
   }


/* check irq flags and process as required */
   if (!spacc_process_irq(spacc)) {
      return IRQ_NONE;
   }
   return IRQ_HANDLED;
}

#ifdef CONFIG_ELP_BLKCIPHER

static DEFINE_SPINLOCK(spacc_sync_lock);

struct spacc_sync_data {
	atomic_t requested;
	atomic_t running;
} spacc_sync_data;

static inline void spacc_request_sync(void)
{
	atomic_inc(&spacc_sync_data.requested);
}

static inline bool spacc_get_sync(void)
{
	bool rc = false;

	if (atomic_read(&spacc_sync_data.requested)) {
		unsigned long flags;
		spin_lock_irqsave(&spacc_sync_lock, flags);
		rc = atomic_read(&spacc_sync_data.requested) &&
			!atomic_read(&spacc_sync_data.running);
		if (rc) {
			atomic_set(&spacc_sync_data.requested, 0);
			atomic_set(&spacc_sync_data.running, 1);
		}
		spin_unlock_irqrestore(&spacc_sync_lock, flags);
	}
	return rc;
}

static inline void spacc_put_sync(void)
{
	atomic_dec(&spacc_sync_data.running);
}

#endif /* CONFIG_ELP_BLKCIPHER */

/* callback function to initialize tasklet running */
static void spacc_stat_process(spacc_device *spacc)
{
   struct spacc_priv *priv = container_of(spacc, struct spacc_priv, spacc);

#ifdef CONFIG_ELP_BLKCIPHER
   spacc_request_sync();
#endif /* CONFIG_ELP_BLKCIPHER */
   /* run tasklet to pop jobs off fifo */
   tasklet_schedule(&priv->pop_jobs);
}

static void spacc_cmd_process(spacc_device *spacc, int x)
{
   struct spacc_priv *priv = container_of(spacc, struct spacc_priv, spacc);

#ifdef CONFIG_ELP_BLKCIPHER
   spacc_request_sync();
#endif /* CONFIG_ELP_BLKCIPHER */
   /* run tasklet to pop jobs off fifo */
   tasklet_schedule(&priv->pop_jobs);

}

void spacc_pop_jobs (unsigned long data)
{
#ifdef CONFIG_ELP_BLKCIPHER
   if (spacc_get_sync()) {
#endif /* CONFIG_ELP_BLKCIPHER */
      struct spacc_priv * priv =  (struct spacc_priv *)data;
      spacc_device *spacc = &priv->spacc;
      int num;

      // decrement the WD CNT here since now we're actually going to respond to the IRQ completely
      if (spacc->wdcnt) {
         --(spacc->wdcnt);
      }

      spacc_pop_packets(spacc, &num);
#ifdef CONFIG_ELP_BLKCIPHER
      spacc_put_sync();
   }
#endif /* CONFIG_ELP_BLKCIPHER */
}
EXPORT_SYMBOL(spacc_pop_jobs);

static int __devinit spacc_probe(struct platform_device *pdev)
{
   void *baseaddr;
   struct resource *mem, *irq;
   int x, err, oldmode;
   struct spacc_priv   *priv;
   pdu_info     info;

   dev_info(&pdev->dev, "probe called!\n");

   mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
   irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
   if (!mem || !irq) {
      dev_err(&pdev->dev, "no memory/irq resource for spacc\n");
      return -ENXIO;
   }

   priv = devm_kzalloc(&pdev->dev, sizeof *priv, GFP_KERNEL);
   if (!priv) {
      dev_err(&pdev->dev, "no memory for spacc private data\n");
      return -ENOMEM;
   }

   printk("spacc_probe: Device at %08lx(%08lx) of size %lu bytes\n", (unsigned long)mem->start, (unsigned long)mem->end, (unsigned long)resource_size(mem));

#ifdef PCI_INDIRECT
   baseaddr = mem->start;
   printk("Using indirect mode\n");
#else
   baseaddr = devm_ioremap_nocache(&pdev->dev, mem->start, resource_size(mem));
#endif
   if (!baseaddr) {
      dev_err(&pdev->dev, "unable to map iomem\n");
      return -ENXIO;
   }

   x = pdev->id;
   dev_info(&pdev->dev, "EPN %04X : virt [%d] \n", (x >> 16) & 0xFFFF, x & 0xF);

   pdu_get_version(baseaddr, &info);
   if (pdev->dev.platform_data) {
      pdu_info *parent_info = pdev->dev.platform_data;
      memcpy(&info.pdu_config, &parent_info->pdu_config, sizeof info.pdu_config);
   }

   err = spacc_init (baseaddr, &priv->spacc, &info);
   if (err != CRYPTO_OK) {
      printk("spacc_probe::Failed to initialize device %d...\n", x);
      return -ENXIO;
   }

   spin_lock_init(&priv->hw_lock);

   spacc_irq_glbl_disable (&priv->spacc);

   tasklet_init(&priv->pop_jobs, spacc_pop_jobs, (unsigned long)priv);

   platform_set_drvdata(pdev, priv);

   /* Determine configured maximum message length. */
   priv->max_msg_len = priv->spacc.config.max_msg_size;

   if (devm_request_irq(&pdev->dev, irq->start, spacc_irq_handler, IRQF_SHARED, dev_name(&pdev->dev), &pdev->dev)) {
      dev_err(&pdev->dev, "failed to request IRQ\n");
      return -EBUSY;
   }

   /* Perform autodetect in no_latency=1 mode */
      priv->spacc.irq_cb_stat = spacc_stat_process;
      priv->spacc.irq_cb_cmdx = spacc_cmd_process;
      oldmode = priv->spacc.op_mode;
      priv->spacc.op_mode     = SPACC_OP_MODE_IRQ;

      spacc_irq_stat_enable (&priv->spacc, 1);
      spacc_irq_cmdx_enable(&priv->spacc, 0, 1);
      spacc_irq_stat_wd_disable (&priv->spacc);
      spacc_irq_glbl_enable (&priv->spacc);

#ifndef MAKEAVECTOR
      spacc_autodetect(&priv->spacc);
#endif
      priv->spacc.op_mode = oldmode;

   /* register irq callback function */
   if (no_latency) {
      // used to set lower latency mode on newer SPAcc device v4.11 and up
      // set above during autodetect
      priv->spacc.op_mode     = SPACC_OP_MODE_IRQ;
      printk("spacc:: Using low latency IRQ mode\n");
   } else {
      if (priv->spacc.op_mode == SPACC_OP_MODE_IRQ) {
         priv->spacc.irq_cb_stat = spacc_stat_process;
         priv->spacc.irq_cb_cmdx = spacc_cmd_process;

         spacc_irq_stat_enable (&priv->spacc, 1);
         spacc_irq_cmdx_enable(&priv->spacc, 0, 1);
         spacc_irq_glbl_enable (&priv->spacc);
      } else {
         priv->spacc.irq_cb_stat    = spacc_stat_process;
         priv->spacc.irq_cb_stat_wd = spacc_stat_process;

         spacc_irq_stat_enable (&priv->spacc, priv->spacc.config.ideal_stat_level);
         spacc_irq_cmdx_disable(&priv->spacc, 0);
         spacc_irq_stat_wd_enable (&priv->spacc);
         spacc_irq_glbl_enable (&priv->spacc);

         /* enable the wd */
         spacc_set_wd_count(&priv->spacc, priv->spacc.config.wd_timer = timer);
      }
   }

   /* Context manager test */
   err = ctx_manager_test (&priv->spacc);

   // unlock normal
   if (priv->spacc.config.is_hsm_shared && priv->spacc.config.is_secure_port) {
      uint32_t t;
      t = pdu_io_read32(baseaddr + SPACC_REG_SECURE_CTRL);
      t &= ~(1UL<<31);
      pdu_io_write32(baseaddr + SPACC_REG_SECURE_CTRL, t);
   }

   return err;
}

static int __devexit spacc_remove(struct platform_device *pdev)
{
   spacc_device *spacc;

   // free test vector memory
   spacc = &((struct spacc_priv *)platform_get_drvdata(pdev))->spacc;
   spacc_fini(spacc);

   /* devm functions do proper cleanup */
   dev_info(&pdev->dev, "removed!\n");

   return 0;
}

#include "elpspacc_irq.h"

static long spacc_kernel_irq_ioctl (struct file *fp, unsigned int cmd, unsigned long arg_)
{
   elpspacc_irq_ioctl io;
   spacc_device *spacc;
   void __user *arg = (void __user *) arg_;
   unsigned long flags;

   if (unlikely (copy_from_user (&io, arg, sizeof (io)))) {
      printk ("spacc_irq_ioctl::Cannot copy ioctl buffer from user\n");
      return -EFAULT;
   }

   spacc = get_spacc_device_by_epn (io.spacc_epn, io.spacc_virt);
   if (!spacc) {
      printk("spacc_irq_ioctl::Cannot find SPAcc %zx/%zx\n", io.spacc_epn, io.spacc_virt);
      return -EIO;
   }

   if (io.command == SPACC_IRQ_CMD_SET) {
      // lock spacc
      PDU_LOCK(&spacc->lock, flags);

      // first disable everything
      spacc_irq_stat_disable(spacc);
      spacc_irq_cmdx_disable(spacc, 0);
      spacc_irq_stat_wd_disable (spacc);
      spacc_irq_glbl_disable (spacc);

      if (io.irq_mode == SPACC_IRQ_MODE_WD) {
         // set WD mode
         spacc->irq_cb_stat    = spacc_stat_process;
         spacc->irq_cb_stat_wd = spacc_stat_process;
         spacc->irq_cb_cmdx    = NULL;

         spacc_irq_stat_enable (spacc, io.stat_value ? io.stat_value : spacc->config.ideal_stat_level);
         spacc_irq_stat_wd_enable (spacc);
         spacc_set_wd_count(spacc, io.wd_value ? io.wd_value : spacc->config.wd_timer);
         spacc->op_mode = SPACC_OP_MODE_WD;
      } else {
         // set STEP mode
         spacc->irq_cb_stat    = spacc_stat_process;
         spacc->irq_cb_cmdx    = spacc_cmd_process;
         spacc->irq_cb_stat_wd = NULL;

         spacc_irq_stat_enable(spacc, io.stat_value ? io.stat_value : 1);
         spacc_irq_cmdx_enable(spacc, 0, io.cmd_value ? io.cmd_value : 1);
         spacc->op_mode = SPACC_OP_MODE_IRQ;
      }
      spacc_irq_glbl_enable (spacc);
      PDU_UNLOCK(&spacc->lock, flags);
   }

   return 0;
}

static struct file_operations spacc_kernel_irq_fops = {
   .owner = THIS_MODULE,
   .unlocked_ioctl = spacc_kernel_irq_ioctl,
};

static struct miscdevice spaccirq_device = {
   .minor = MISC_DYNAMIC_MINOR,
   .name = "spaccirq",
   .fops = &spacc_kernel_irq_fops,
};

static struct platform_driver spacc_driver = {
   .probe  = spacc_probe,
   .remove = __devexit_p(spacc_remove),
   .driver = {
      .name  = "spacc",
      .owner = THIS_MODULE
   },
};

static int __init spacc_mod_init (void)
{
   int err;

   err = misc_register (&spaccirq_device);
   if (err) {
      return err;
   }

   err = platform_driver_register(&spacc_driver);
   if (err) {
      misc_deregister (&spaccirq_device);
   }
   return err;
}

static void __exit spacc_mod_exit (void)
{
   misc_deregister (&spaccirq_device);
   platform_driver_unregister(&spacc_driver);
}

MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("Elliptic Technologies Inc.");
module_init (spacc_mod_init);
module_exit (spacc_mod_exit);

// export wrapped library functions
EXPORT_SYMBOL (spacc_open);
EXPORT_SYMBOL (spacc_clone_handle);
EXPORT_SYMBOL (spacc_close);
//EXPORT_SYMBOL (spacc_status);
EXPORT_SYMBOL (spacc_write_context);
EXPORT_SYMBOL (spacc_read_context);
EXPORT_SYMBOL (spacc_error_msg);
EXPORT_SYMBOL (spacc_set_operation);
EXPORT_SYMBOL (spacc_set_key_exp);
EXPORT_SYMBOL (spacc_set_auxinfo);
EXPORT_SYMBOL (spacc_packet_enqueue_ddt);
EXPORT_SYMBOL (spacc_packet_dequeue);
EXPORT_SYMBOL (spacc_virtual_set_weight);
EXPORT_SYMBOL (spacc_virtual_request_rc4);
EXPORT_SYMBOL (spacc_pop_packets);
EXPORT_SYMBOL (spacc_load_skp);
EXPORT_SYMBOL (spacc_dump_ctx);
EXPORT_SYMBOL (spacc_set_wd_count);

EXPORT_SYMBOL(spacc_irq_cmdx_enable);
EXPORT_SYMBOL(spacc_irq_cmdx_disable);
EXPORT_SYMBOL(spacc_irq_stat_enable);
EXPORT_SYMBOL(spacc_irq_stat_disable);
EXPORT_SYMBOL(spacc_irq_stat_wd_enable);
EXPORT_SYMBOL(spacc_irq_stat_wd_disable);
EXPORT_SYMBOL(spacc_irq_rc4_dma_enable);
EXPORT_SYMBOL(spacc_irq_rc4_dma_disable);
EXPORT_SYMBOL(spacc_irq_glbl_enable);
EXPORT_SYMBOL(spacc_irq_glbl_disable);
EXPORT_SYMBOL(spacc_process_irq);

EXPORT_SYMBOL(spacc_compute_xcbc_key);
EXPORT_SYMBOL(spacc_isenabled);

// used by RE/KEP
EXPORT_SYMBOL (spacc_ctx_request);
EXPORT_SYMBOL (spacc_ctx_release);

int spacc_endian;
module_param(spacc_endian, int, 0);
MODULE_PARM_DESC(spacc_endian, "Endianess of data transfers (0==little)");
module_param_named(timer, timer, int, S_IRUSR | S_IWUSR);
MODULE_PARM_DESC(timer, "Watchdog timer value (default==0xAFC8 which is 1ms@45MHz)");

module_param_named(no_latency, no_latency, int, S_IRUSR | S_IWUSR);
MODULE_PARM_DESC(no_latency, "Set to 1 to have a low latency IRQ mechanism");

EXPORT_SYMBOL(spacc_endian);
