#define DRVNAME "trng3"

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/hw_random.h>
#include <linux/ctype.h>

#include "trng3_driver.h"

/*
 * Return true iff a reseed has been started but not yet acknowledged by
 * the interrupt handler.
 */
static bool is_reseeding(struct device *dev)
{
   struct trng3_priv *priv = dev_get_drvdata(dev);
   uint32_t stat = elptrng3_readreg(&priv->trng3, TRNG3_STAT);

   if ((stat >> TRNG3_STAT_RESEEDING) & 1)
      return true;

   if ((stat >> TRNG3_STAT_SRVC_RQST) & 1) {
      /* There is a pending IRQ indicated; need to check ISTAT. */
      uint32_t istat = elptrng3_readreg(&priv->trng3, TRNG3_ISTAT);

      if ((istat >> TRNG3_IRQ_SEED_DONE) & 1)
         return true;
   }

   return false;
}

static bool is_secure(struct device *dev)
{
   struct trng3_priv *priv = dev_get_drvdata(dev);
   u32 stat = elptrng3_readreg(&priv->trng3, TRNG3_STAT);

   return (stat >> TRNG3_STAT_SECURE) & 1;
}

static ssize_t
show_epn(struct device *dev, struct device_attribute *devattr, char *buf)
{
   struct trng3_priv *priv = dev_get_drvdata(dev);

   return sprintf(buf, "0x%.4hx\n", priv->trng3.epn);
}

static ssize_t
show_stepping(struct device *dev, struct device_attribute *devattr, char *buf)
{
   struct trng3_priv *priv = dev_get_drvdata(dev);

   return sprintf(buf, "%hu\n", priv->trng3.stepping);
}

static ssize_t
show_seed(struct device *dev, struct device_attribute *devattr, char *buf)
{
   struct trng3_priv *priv = dev_get_drvdata(dev);
   unsigned char tmp[sizeof priv->nonce_buf];
   unsigned long long seed_start, seed_end;
   size_t i;
   int rc;

   /*
    * If a background reseed is in progress while reading the seed registers,
    * the result may contain a mixture of old and new seed materials (and is
    * therefore bogus).  Moreover, S/W does not necessarily have complete
    * control over when the engine is reseeded so we cannot entirely avoid
    * this situation.
    *
    * Since completely preventing the race is impossible, we will instead
    * validate after the fact whether or not a reseed occurred during our
    * read of the seed registers.  If no reseed occurred, then the seed
    * registers were stable and we return that.  Otherwise, we retry the
    * read.  We implement this by maintaining a counter that increments
    * whenever the engine reseeds, plus some checks in this function to
    * deal with IRQ latency.
    */

   do {
      rc = wait_event_interruptible(priv->reseed_wq, !is_reseeding(dev));
      if (rc < 0)
         return rc;

      device_lock(dev);
      if (is_secure(dev)) {
         /* The seed registers are only readable in promiscuous mode. */
         device_unlock(dev);
         return -EPERM;
      }

      spin_lock_irq(&priv->cfg_lock);
      seed_start = priv->seed_generation;
      spin_unlock_irq(&priv->cfg_lock);

      rc = elptrng3_get_seed(&priv->trng3, tmp);
      device_unlock(dev);

      if (rc < 0)
         return pdu_error_code(rc);

      if (is_reseeding(dev)) {
         seed_end = seed_start + 1;
      } else {
         spin_lock_irq(&priv->cfg_lock);
         seed_end = priv->seed_generation;
         spin_unlock_irq(&priv->cfg_lock);
      }
   } while (seed_start != seed_end);

   for (i = 0; i < sizeof tmp; i++) {
      sprintf(buf+2*i, "%.2hhx\n", tmp[i]);
   }

   return 2*i+1;
}

static ssize_t
show_mode(struct device *dev, struct device_attribute *devattr, char *buf)
{
   if (is_secure(dev))
      return sprintf(buf, "secure\n");
   return sprintf(buf, "promiscuous\n");
}

static ssize_t
store_mode(struct device *dev, struct device_attribute *devattr,
                               const char *buf, size_t count)
{
   struct trng3_priv *priv = dev_get_drvdata(dev);
   bool newmode;

   if (sysfs_streq(buf, "secure"))
      newmode = true;
   else if (sysfs_streq(buf, "promiscuous"))
      newmode = false;
   else
      return -EINVAL;

   device_lock(dev);

   if (is_secure(dev) != newmode) {
      spin_lock_irq(&priv->cfg_lock);
      elptrng3_set_secure(&priv->trng3, newmode);

      /* Pending alarms are invalid after a mode change */
      priv->alarm_reason = 0;
      spin_unlock_irq(&priv->cfg_lock);

      kobject_uevent(&dev->kobj, KOBJ_CHANGE);
   }

   device_unlock(dev);

   return count;
}

static ssize_t
store_seed(struct device *dev, struct device_attribute *devattr,
                               const char *buf, size_t count)
{
   struct trng3_priv *priv = dev_get_drvdata(dev);
   unsigned char tmp[sizeof priv->nonce_buf] = {0};
   size_t i, pos;
   int cur = -1;

   if (count == sizeof tmp) {
      /* Binary input. */
      memcpy(tmp, buf, sizeof tmp);
   } else {
      /* Hexadecimal input. */
      for (i = pos = 0; i < count && pos < sizeof tmp; i++) {
         int rc;

         if (isspace(buf[i]))
            continue;

         rc = hex_to_bin(buf[i]);
         if (rc < 0)
            return -EINVAL;

         if (cur == -1) {
            cur = rc << 4;
         } else {
            tmp[pos++] = cur | rc;
            cur = -1;
         }
      }

      if (pos != sizeof tmp)
         return -EINVAL;

      /* Allow for trailing whitespace. */
      for (; i < count; i++) {
         if (!isspace(buf[i]))
            return -EINVAL;
      }
   }

   device_lock(dev);
   memcpy(priv->nonce_buf, tmp, sizeof priv->nonce_buf);
   priv->nonce_valid = true;
   device_unlock(dev);

   return count;
}

static ssize_t
show_reseed(struct device *dev, struct device_attribute *devattr, char *buf)
{
   struct trng3_priv *priv = dev_get_drvdata(dev);
   u32 stat = elptrng3_readreg(&priv->trng3, TRNG3_STAT);

   stat >>= TRNG3_STAT_RESEED_REASON;
   stat &= (1ul << TRNG3_STAT_RESEED_REASON_BITS) - 1;
   switch (stat) {
   case TRNG3_RESEED_HOST:
      return sprintf(buf, "random (host command)\n");
   case TRNG3_RESEED_PIN:
      return sprintf(buf, "random (external pin)\n");
   case TRNG3_RESEED_NONCE:
      return sprintf(buf, "nonce\n");
   case TRNG3_RESEED_UNSEEDED:
      return sprintf(buf, "unseeded\n");
   }

   return sprintf(buf, "unknown (%d)\n", (int)stat);
}

static int reseed_action_random(struct device *dev)
{
   struct trng3_priv *priv = dev_get_drvdata(dev);
   unsigned long long seed_start, seed_end;
   int rc;

   spin_lock_irq(&priv->cfg_lock);
   seed_start = priv->seed_generation;
   rc = elptrng3_reseed(&priv->trng3, NULL);
   spin_unlock_irq(&priv->cfg_lock);

   if (rc < 0)
      return pdu_error_code(rc);

   do {
      rc = wait_event_killable(priv->reseed_wq, !is_reseeding(dev));
      if (rc < 0)
         return rc;

      spin_lock_irq(&priv->cfg_lock);
      seed_end = priv->seed_generation;
      spin_unlock_irq(&priv->cfg_lock);
   } while (seed_start == seed_end);

   return 0;
}

static int reseed_action_nonce(struct device *dev)
{
   struct trng3_priv *priv = dev_get_drvdata(dev);
   int rc;

   device_lock(dev);
   if (!priv->nonce_valid) {
      device_unlock(dev);
      return -ENODATA;
   }

   /*
    * Nonce reseed does not trigger an IRQ, so we must increment the
    * generation ourselves.
    */

   spin_lock_irq(&priv->cfg_lock);
   rc = elptrng3_reseed(&priv->trng3, priv->nonce_buf);
   if (rc < 0) {
      spin_unlock_irq(&priv->cfg_lock);
      device_unlock(dev);
      return pdu_error_code(rc);
   }

   priv->seed_generation++;
   spin_unlock_irq(&priv->cfg_lock);

   priv->nonce_valid = false;
   device_unlock(dev);

   /*
    * Since a nonce reseed aborts any in-progress
    * random reseed, wake up the waiters.
    */
   wake_up_all(&priv->reseed_wq);
   return 0;
}

static ssize_t
store_reseed(struct device *dev, struct device_attribute *devattr,
                                 const char *buf, size_t count)
{
   int rc = -EINVAL;

   if (sysfs_streq(buf, "random")) {
      rc = reseed_action_random(dev);
   } else if (sysfs_streq(buf, "nonce")) {
      rc = reseed_action_nonce(dev);
   }

   if (rc < 0)
      return rc;
   return count;
}

static ssize_t
show_gen(struct device *dev, struct device_attribute *devattr, char *buf)
{
   struct trng3_priv *priv = dev_get_drvdata(dev);
   unsigned long long val;

   spin_lock_irq(&priv->cfg_lock);
   val = priv->seed_generation;
   spin_unlock_irq(&priv->cfg_lock);

   return sprintf(buf, "%llu\n", val);
}

static ssize_t
show_max_rej(struct device *dev, struct device_attribute *devattr, char *buf)
{
   struct trng3_priv *priv = dev_get_drvdata(dev);
   u32 val;

   val   = elptrng3_readreg(&priv->trng3, TRNG3_SMODE);
   val >>= TRNG3_SMODE_MAX_REJECTS;
   val  &= (1ul<<TRNG3_SMODE_MAX_REJECTS_BITS)-1;

   return sprintf(buf, "%lu\n", (unsigned long)val);
}

static ssize_t
store_max_rej(struct device *dev, struct device_attribute *devattr,
                                  const char *buf, size_t count)
{
   struct trng3_priv *priv = dev_get_drvdata(dev);
   unsigned val;
   int rc;

   rc = kstrtouint(buf, 0, &val);
   if (rc < 0)
      return rc;

   spin_lock_irq(&priv->cfg_lock);
   rc = elptrng3_set_max_rejects(&priv->trng3, val);
   spin_unlock_irq(&priv->cfg_lock);

   if (rc != 0)
      return -ERANGE;

   return count;
}

static ssize_t
show_outlen(struct device *dev, struct device_attribute *devattr, char *buf)
{
   struct trng3_priv *priv = dev_get_drvdata(dev);
   u32 stat = elptrng3_readreg(&priv->trng3, TRNG3_STAT);

   if ((stat >> TRNG3_STAT_R256) & 1)
      return sprintf(buf, "32\n");
   return sprintf(buf, "16\n");
}

static ssize_t
store_outlen(struct device *dev, struct device_attribute *devattr,
                                 const char *buf, size_t count)
{
   struct trng3_priv *priv = dev_get_drvdata(dev);
   unsigned val;
   int rc;

   rc = kstrtouint(buf, 0, &val);
   if (rc < 0)
      return rc;

   device_lock(dev);

   spin_lock_irq(&priv->cfg_lock);
   rc = elptrng3_set_output_len(&priv->trng3, val);
   spin_unlock_irq(&priv->cfg_lock);

   if (rc < 0) {
      device_unlock(dev);
      return pdu_error_code(rc);
   }

   /* Eat the next random value since its size will be bogus. */
   while (elptrng3_get_random(&priv->trng3, NULL) == CRYPTO_INPROGRESS)
      continue;

   device_unlock(dev);

   return count;
}

/*
 * Obtain a single random value from the TRNG.  Returns the number of
 * bytes written, or a negative error code.
 */
static int trng3_crank_rng(struct device *dev, unsigned char *out)
{
   struct trng3_priv *priv = dev_get_drvdata(dev);
   int rc;

   device_lock(dev);
retry:
   rc = elptrng3_get_random(&priv->trng3, out);
   switch (rc) {
   case 0:
      elptrng3_writereg(&priv->trng3, TRNG3_CTRL, TRNG3_CMD_GEN_RAND);
   case CRYPTO_INPROGRESS:
      cond_resched();
      goto retry;
   }

   if (rc < 0) {
      device_unlock(dev);
      return pdu_error_code(rc);
   }

   elptrng3_writereg(&priv->trng3, TRNG3_CTRL, TRNG3_CMD_GEN_RAND);
   device_unlock(dev);

   return rc;
}

static ssize_t trng3_read(struct device *dev, void *out, size_t outlen)
{
   unsigned char buf[ELPTRNG3_MAXLEN];
   size_t ret;
   int rc;

   if (outlen > INT_MAX)
      return -EINVAL;

   for (ret = 0; ret < outlen;) {
      rc = trng3_crank_rng(dev, buf);
      if (rc < 0) {
         /* Only return an error if we got no data. */
         if (!ret)
            return rc;
         break;
      }

      if (rc > outlen-ret)
         rc = outlen-ret;

      memcpy(out+ret, buf, rc);
      ret += rc;
   }

   return ret;
}

static ssize_t
show_random(struct device *dev, struct device_attribute *devattr, char *buf)
{
   unsigned char rbuf[ELPTRNG3_MAXLEN];
   int i, rlen;

   rlen = trng3_crank_rng(dev, rbuf);
   if (rlen < 0)
      return rlen;

   for (i = 0; i < rlen; i++) {
      sprintf(buf+2*i, "%.2hhx\n", rbuf[i]);
   }

   return 2*i+1;
}

static ssize_t
show_rqst(struct device *dev, struct device_attribute *devattr, char *buf)
{
   struct trng3_priv *priv = dev_get_drvdata(dev);
   unsigned long long val;

   val  = elptrng3_readreg(&priv->trng3, TRNG3_AUTO_RQST);
   val &= (1ul << TRNG3_AUTO_RQST_RQSTS_BITS) - 1;
   val *= TRNG3_AUTO_RQST_RESOLUTION;

   return sprintf(buf, "%llu\n", val);
}

static ssize_t
show_age(struct device *dev, struct device_attribute *devattr, char *buf)
{
   struct trng3_priv *priv = dev_get_drvdata(dev);
   unsigned long long val;

   val  = elptrng3_readreg(&priv->trng3, TRNG3_AUTO_AGE);
   val &= (1ul << TRNG3_AUTO_AGE_AGE_BITS) - 1;
   val *= TRNG3_AUTO_AGE_RESOLUTION;

   return sprintf(buf, "%llu\n", val);
}

static ssize_t
store_rqst(struct device *dev, struct device_attribute *devattr,
                               const char *buf, size_t count)
{
   struct trng3_priv *priv = dev_get_drvdata(dev);
   unsigned long long val;
   int rc;

   rc = kstrtoull(buf, 0, &val);
   if (rc < 0)
      return rc;

   rc = elptrng3_set_request_reminder(&priv->trng3, val);
   if (rc < 0)
      return pdu_error_code(rc);

   return count;
}

static ssize_t
store_age(struct device *dev, struct device_attribute *devattr,
                              const char *buf, size_t count)
{
   struct trng3_priv *priv = dev_get_drvdata(dev);
   unsigned long long val;
   int rc;

   rc = kstrtoull(buf, 0, &val);
   if (rc < 0)
      return rc;

   rc = elptrng3_set_age_reminder(&priv->trng3, val);
   if (rc < 0)
      return pdu_error_code(rc);

   return count;
}

static ssize_t
show_alarm(struct device *dev, struct device_attribute *devattr, char *buf)
{
   struct trng3_priv *priv = dev_get_drvdata(dev);
   unsigned alarm_reason = 0;

   spin_lock_irq(&priv->cfg_lock);
   if (priv->alarm_generation == priv->seed_generation)
      alarm_reason = priv->alarm_reason;
   spin_unlock_irq(&priv->cfg_lock);

   switch (alarm_reason) {
   case 0:
      return sprintf(buf, "none\n");
   case TRNG3_IRQ_AGE_ALARM:
      return sprintf(buf, "age\n");
   case TRNG3_IRQ_RQST_ALARM:
      return sprintf(buf, "request\n");
   }

   return sprintf(buf, "unknown\n");
}

static DEVICE_ATTR(epn,              0444, show_epn,      NULL);
static DEVICE_ATTR(stepping,         0444, show_stepping, NULL);
static DEVICE_ATTR(seed,             0600, show_seed,     store_seed);
static DEVICE_ATTR(seed_generation,  0444, show_gen,      NULL);
static DEVICE_ATTR(reseed_action,    0600, show_reseed,   store_reseed);
static DEVICE_ATTR(mode,             0644, show_mode,     store_mode);
static DEVICE_ATTR(reject_threshold, 0644, show_max_rej,  store_max_rej);
static DEVICE_ATTR(output_len,       0644, show_outlen,   store_outlen);
static DEVICE_ATTR(random,           0444, show_random,   NULL);
static DEVICE_ATTR(wd_requests,      0644, show_rqst,     store_rqst);
static DEVICE_ATTR(wd_cycles,        0644, show_age,      store_age);
static DEVICE_ATTR(wd_alarm,         0444, show_alarm,    NULL);

static const struct attribute_group trng3_attr_group = {
   .attrs = (struct attribute *[]) {
      &dev_attr_epn.attr,
      &dev_attr_stepping.attr,
      &dev_attr_seed.attr,
      &dev_attr_seed_generation.attr,
      &dev_attr_reseed_action.attr,
      &dev_attr_mode.attr,
      &dev_attr_reject_threshold.attr,
      &dev_attr_output_len.attr,
      &dev_attr_random.attr,
      &dev_attr_wd_requests.attr,
      &dev_attr_wd_cycles.attr,
      &dev_attr_wd_alarm.attr,
      NULL
   },
};

static irqreturn_t trng3_irq(int irq, void *dev)
{
   struct trng3_priv *priv = dev_get_drvdata(dev);
   u32 istat = elptrng3_readreg(&priv->trng3, TRNG3_ISTAT);
   irqreturn_t ret = IRQ_NONE;
   u32 ack = 0;

   if (istat & TRNG3_IRQ_AGE_ALARM_MASK) {
      spin_lock(&priv->cfg_lock);
      priv->alarm_generation = priv->seed_generation;
      priv->alarm_reason = TRNG3_IRQ_AGE_ALARM;
      spin_unlock(&priv->cfg_lock);

      ack |= TRNG3_IRQ_AGE_ALARM_MASK;
      schedule_work(&priv->alarm_work);
   }

   if (istat & TRNG3_IRQ_RQST_ALARM_MASK) {
      spin_lock(&priv->cfg_lock);
      priv->alarm_generation = priv->seed_generation;
      priv->alarm_reason = TRNG3_IRQ_RQST_ALARM;
      spin_unlock(&priv->cfg_lock);

      ack |= TRNG3_IRQ_RQST_ALARM_MASK;
      schedule_work(&priv->alarm_work);
   }

   if (istat & TRNG3_IRQ_SEED_DONE_MASK) {
      spin_lock(&priv->cfg_lock);
      priv->seed_generation++;
      spin_unlock(&priv->cfg_lock);

      ack |= TRNG3_IRQ_SEED_DONE_MASK;
   }

   if (ack) {
      elptrng3_writereg(&priv->trng3, TRNG3_ISTAT, ack);
      if (ack & TRNG3_IRQ_SEED_DONE_MASK)
         wake_up_all(&priv->reseed_wq);

      ret = IRQ_HANDLED;
   }

   return ret;
}

static int
trng3_hwrng_read(struct hwrng *rng, void *data, size_t max, bool wait)
{
   struct device *dev = (void *)rng->priv;

   return trng3_read(dev, data, max);
}

static int __devinit trng3_setup_hwrng(struct device *dev)
{
   struct trng3_priv *priv = dev_get_drvdata(dev);

   if (!IS_ENABLED(CONFIG_HW_RANDOM))
      return 0;

   priv->hwrng = (struct hwrng) {
      .name = DRVNAME,
      .read = trng3_hwrng_read,
      .priv = (unsigned long)dev,
   };

   return hwrng_register(&priv->hwrng);
}

static void trng3_alarm_notify(struct work_struct *work)
{
   struct trng3_priv *priv = container_of(work, struct trng3_priv, alarm_work);
   struct device *dev = (void *)priv->hwrng.priv;

   kobject_uevent(&dev->kobj, KOBJ_CHANGE);
}

static void __devinit trng3_describe_device(struct device *dev)
{
   struct trng3_priv *priv = dev_get_drvdata(dev);

   dev_info(dev, "EPN-%.4hx Random Number Generator (stepping: %hx)\n",
                 priv->trng3.epn, priv->trng3.stepping);

   if (priv->trng3.output_len > 16)
      dev_info(dev, "Supports %u-bit output mode\n", 8u*priv->trng3.output_len);
   if (priv->trng3.rings_avail)
      dev_info(dev, "Supports true random reseed function\n");
   if (priv->trng3.diag_level)
      dev_info(dev, "Supports level %hu diagnostics\n", priv->trng3.diag_level);
}

static int __devinit trng3_probe(struct platform_device *pdev)
{
   struct resource *mem, *irq;
   struct trng3_priv *priv;
   u32 __iomem *regbase;
   int rc;

   mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
   if (!mem || resource_size(mem) < 0x70)
      return -EINVAL;

   irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
   if (!irq)
      return -EINVAL;

   if (!devm_request_mem_region(&pdev->dev, mem->start, resource_size(mem),
                                            DRVNAME "-regs"))
      return -EADDRNOTAVAIL;

   priv = devm_kzalloc(&pdev->dev, sizeof *priv, GFP_KERNEL);
   if (!priv)
      return -ENOMEM;

   spin_lock_init(&priv->cfg_lock);
   spin_lock_init(&priv->iap_lock);
   INIT_WORK(&priv->alarm_work, trng3_alarm_notify);
   init_waitqueue_head(&priv->reseed_wq);
   platform_set_drvdata(pdev, priv);

   regbase = devm_ioremap_nocache(&pdev->dev, mem->start, resource_size(mem));
   if (!regbase)
      return -ENOMEM;
   elptrng3_setup(&priv->trng3, regbase);

   rc = devm_request_irq(&pdev->dev, irq->start, trng3_irq, IRQF_SHARED,
                                     dev_name(&pdev->dev), &pdev->dev);
   if (rc < 0)
      return rc;

   rc = sysfs_create_group(&pdev->dev.kobj, &trng3_attr_group);
   if (rc < 0)
      return rc;

   rc = trng3_create_debug_attrs(&pdev->dev);
   if (rc < 0) {
      sysfs_remove_group(&pdev->dev.kobj, &trng3_attr_group);
      return rc;
   }

   /* Notify userspace of new attributes */
   kobject_uevent(&pdev->dev.kobj, KOBJ_CHANGE);

   trng3_describe_device(&pdev->dev);

   spin_lock_irq(&priv->cfg_lock);
   elptrng3_set_output_len(&priv->trng3, 32);
   elptrng3_enable_irqs(&priv->trng3, TRNG3_IRQ_GLBL_EN_MASK
                                    | TRNG3_IRQ_RQST_ALARM_MASK
                                    | TRNG3_IRQ_AGE_ALARM_MASK
                                    | TRNG3_IRQ_SEED_DONE_MASK);
   spin_unlock_irq(&priv->cfg_lock);

   /* Register H/W random interfaces if TRNG functions are available. */
   if (priv->trng3.rings_avail) {
      rc = trng3_setup_hwrng(&pdev->dev);
      if (rc < 0) {
         dev_err(&pdev->dev, "failed to setup hw_random interfaces\n");
         /* non-fatal */
      }
   }

   return 0;
}

static int __devexit trng3_remove(struct platform_device *pdev)
{
   struct trng3_priv *priv = platform_get_drvdata(pdev);

   if (IS_ENABLED(CONFIG_HW_RANDOM))
      hwrng_unregister(&priv->hwrng);

   spin_lock_irq(&priv->cfg_lock);
   elptrng3_disable_irqs(&priv->trng3, -1);
   spin_unlock_irq(&priv->cfg_lock);

   trng3_remove_debug_attrs(&pdev->dev);
   sysfs_remove_group(&pdev->dev.kobj, &trng3_attr_group);

   return 0;
}

static struct platform_driver trng3_driver = {
   .probe  = trng3_probe,
   .remove = __devexit_p(trng3_remove),

   .driver = {
      .name   = DRVNAME,
      .owner  = THIS_MODULE,
   },
};

static int __init trng3_init(void)
{
   return platform_driver_register(&trng3_driver);
}
module_init(trng3_init);

static void __exit trng3_exit(void)
{
   platform_driver_unregister(&trng3_driver);
}
module_exit(trng3_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Elliptic Technologies Inc.");
