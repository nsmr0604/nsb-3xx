#include <linux/device.h>
#include <linux/delay.h>
#include <linux/ctype.h>

#include "trng3_driver.h"

enum {
   TRNG3_IAP_REJECTS0,
   TRNG3_IAP_REJECTS1,
   TRNG3_IAP_REJECTS2,
   TRNG3_IAP_REJECTS3,
   TRNG3_IAP_REJECTS4,
   TRNG3_IAP_REJECTS5,

   TRNG3_IAP_BIT_CNTR_1_0 = 0x8,
   TRNG3_IAP_BIT_CNTR_3_2,
   TRNG3_IAP_BIT_CNTR_5_4,

   TRNG3_IAP_MUX_SEL = 0xc,
   TRNG3_IAP_BG_DISABLE,
   TRNG3_IAP_SIMULT_CNTRS,

   TRNG3_IAP_COND_LD_LO = 0x10,
   TRNG3_IAP_COND_LD_HI,

   TRNG3_IAP_BIT_GEN_1ST = 0x13,
   TRNG3_IAP_DPD_CLR,
   TRNG3_IAP_DPD_MEM_PTR,
   TRNG3_IAP_DPD_RD_DATA,
   TRNG3_IAP_DPD_STAT,
   TRNG3_IAP_DPD_FORCE,
};

enum {
   DPD_STATE_CLR,
   DPD_STATE_READING,
   DPD_STATE_CHECK,
   DPD_STATE_COLLECT,
   DPD_STATE_IDLE,
};

struct ring_attribute {
   struct device_attribute devattr;
   unsigned ring_id;
};
#define RING_ATTR_SUFFIX(name_prefix, name_suffix, id, mode, show, store) \
   struct ring_attribute ring_attr_##name_prefix##id##name_suffix = { \
      .devattr = __ATTR(name_prefix##id##name_suffix, mode, show, store), \
      .ring_id = id \
   }

#define RING_ATTR(name, id, mode, show, store) \
   RING_ATTR_SUFFIX(name, /*empty*/, id, mode, show, store)

static u32 trng3_indirect_read(struct device *dev, u32 addr)
{
   struct trng3_priv *priv = dev_get_drvdata(dev);
   u32 cmd = 1ul << TRNG3_IA_CMD_GO, ret;

   spin_lock(&priv->iap_lock);
   elptrng3_writereg(&priv->trng3, TRNG3_IA_ADDR, addr);
   elptrng3_writereg(&priv->trng3, TRNG3_IA_CMD,  cmd);
   ret = elptrng3_readreg(&priv->trng3, TRNG3_IA_RDATA);
   spin_unlock(&priv->iap_lock);

   return ret;
}

static void trng3_indirect_write(struct device *dev, u32 addr, u32 val)
{
   struct trng3_priv *priv = dev_get_drvdata(dev);
   u32 cmd = (1ul << TRNG3_IA_CMD_GO) | (1ul << TRNG3_IA_CMD_WRITE);

   spin_lock(&priv->iap_lock);
   elptrng3_writereg(&priv->trng3, TRNG3_IA_WDATA, val);
   elptrng3_writereg(&priv->trng3, TRNG3_IA_ADDR,  addr);
   elptrng3_writereg(&priv->trng3, TRNG3_IA_CMD,   cmd);

   mmiowb();
   spin_unlock(&priv->iap_lock);
}

static bool is_secure(struct device *dev)
{
   struct trng3_priv *priv = dev_get_drvdata(dev);
   u32 stat = elptrng3_readreg(&priv->trng3, TRNG3_STAT);

   return (stat >> TRNG3_STAT_SECURE) & 1;
}

static unsigned ring_attr_id(struct device_attribute *devattr)
{
   struct ring_attribute *ringattr;

   ringattr = container_of(devattr, struct ring_attribute, devattr);
   BUG_ON(ringattr->ring_id > 5);

   return ringattr->ring_id;
}

static ssize_t
show_rejects(struct device *dev, struct device_attribute *devattr, char *buf)
{
   u32 addr, val;

   device_lock(dev);
   if (is_secure(dev)) {
      device_unlock(dev);
      return -EPERM;
   }

   addr = TRNG3_IAP_REJECTS0 + 4*ring_attr_id(devattr);
   val = trng3_indirect_read(dev, addr);
   device_unlock(dev);

   return sprintf(buf, "%lu\n", (unsigned long)val);
}

static ssize_t
show_contrib(struct device *dev, struct device_attribute *devattr, char *buf)
{
   unsigned ring_id = ring_attr_id(devattr);
   u32 addr, val;

   device_lock(dev);
   if (is_secure(dev)) {
      device_unlock(dev);
      return -EPERM;
   }

   addr  = TRNG3_IAP_BIT_CNTR_1_0 + 2*(ring_id & ~1ul);
   val   = trng3_indirect_read(dev, addr);
   device_unlock(dev);

   val >>= 16*(ring_id & 1);
   val  &= 0x1ff;

   return sprintf(buf, "%lu\n", (unsigned long)val);
}

static ssize_t
show_mux(struct device *dev, struct device_attribute *devattr, char *buf)
{
   unsigned ring_id = ring_attr_id(devattr);
   u32 val;

   device_lock(dev);
   if (is_secure(dev)) {
      device_unlock(dev);
      return -EPERM;
   }

   val = trng3_indirect_read(dev, TRNG3_IAP_MUX_SEL);
   device_unlock(dev);

   val >>= 2*ring_id;
   val  &= 3;

   return sprintf(buf, "%lu\n", (unsigned long)val);
}

static ssize_t
show_enable(struct device *dev, struct device_attribute *devattr, char *buf)
{
   unsigned ring_id = ring_attr_id(devattr);
   u32 val;

   device_lock(dev);
   if (is_secure(dev)) {
      device_unlock(dev);
      return -EPERM;
   }

   val = trng3_indirect_read(dev, TRNG3_IAP_BG_DISABLE);
   device_unlock(dev);

   val >>= ring_id;
   if (val & 1) {
      return sprintf(buf, "no\n");
   }

   return sprintf(buf, "yes\n");
}

static ssize_t
store_enable(struct device *dev, struct device_attribute *devattr,
                                 const char *buf, size_t count)
{
   unsigned ring_id = ring_attr_id(devattr);
   u32 val, mask = 1ul << ring_id;
   bool enable;
   int rc;

   rc = strtobool(buf, &enable);
   if (rc < 0)
      return rc;

   device_lock(dev);
   if (is_secure(dev)) {
      device_unlock(dev);
      return -EPERM;
   }

   val = trng3_indirect_read(dev, TRNG3_IAP_BG_DISABLE);

   if (enable)
      val &= ~mask;
   else
      val |= mask;

   trng3_indirect_write(dev, TRNG3_IAP_BG_DISABLE, val);
   device_unlock(dev);

   return count;
}

static ssize_t
show_cond_in(struct device *dev, struct device_attribute *devattr, char *buf)
{
   u32 lo, hi;

   device_lock(dev);
   if (is_secure(dev)) {
      device_unlock(dev);
      return -EPERM;
   }

   lo = trng3_indirect_read(dev, TRNG3_IAP_COND_LD_LO);
   hi = trng3_indirect_read(dev, TRNG3_IAP_COND_LD_HI);
   device_unlock(dev);

   return sprintf(buf, "%.8lx%.8lx\n", (unsigned long)hi, (unsigned long)lo);
}

static ssize_t
show_first_gen(struct device *dev, struct device_attribute *devattr, char *buf)
{
   u32 val;

   device_lock(dev);
   if (is_secure(dev)) {
      device_unlock(dev);
      return -EPERM;
   }

   val = trng3_indirect_read(dev, TRNG3_IAP_BIT_GEN_1ST);
   device_unlock(dev);

   return sprintf(buf, "%lu\n", (unsigned long)val);
}

static ssize_t
show_simult(struct device *dev, struct device_attribute *devattr, char *buf)
{
   u32 val, mask;
   int counter, offset;

   counter = devattr->attr.name[strlen(devattr->attr.name)-1];
   if (!isdigit(counter))
      return -EINVAL;
   counter -= '0';

   /* First counter is 2 simultaneous generations. */
   if (counter >= 2 && counter <= 4) {
      offset = 8*(counter-2);
      mask = 0xff;
   } else if (counter >= 5 && counter <= 6) {
      offset = 24 + 4*(counter-5);
      mask = 0xf;
   } else {
      return -EINVAL;
   }

   device_lock(dev);
   if (is_secure(dev)) {
      device_unlock(dev);
      return -EPERM;
   }

   val = trng3_indirect_read(dev, TRNG3_IAP_SIMULT_CNTRS);
   device_unlock(dev);

   val >>= offset;
   val &= mask;

   return sprintf(buf, "%lu\n", (unsigned long)val);
}

static ssize_t
store_dpd_clr(struct device *dev, struct device_attribute *devattr,
                                  const char *buf, size_t count)
{
   device_lock(dev);
   if (is_secure(dev)) {
      device_unlock(dev);
      return -EPERM;
   }

   trng3_indirect_write(dev, TRNG3_IAP_DPD_CLR, 1);
   device_unlock(dev);

   return count;
}

static ssize_t
store_dpd_force(struct device *dev, struct device_attribute *devattr,
                                    const char *buf, size_t count)
{
   device_lock(dev);
   if (is_secure(dev)) {
      device_unlock(dev);
      return -EPERM;
   }

   trng3_indirect_write(dev, TRNG3_IAP_DPD_FORCE, 1);
   device_unlock(dev);

   return count;
}

static ssize_t
show_dpd_state(struct device *dev, struct device_attribute *devattr, char *buf)
{
   u32 val;

   device_lock(dev);
   if (is_secure(dev)) {
      device_unlock(dev);
      return -EPERM;
   }

   val = trng3_indirect_read(dev, TRNG3_IAP_DPD_STAT);
   device_unlock(dev);

   switch ((val >> 1) & 0x1f) {
   case 1ul << DPD_STATE_IDLE:
      return sprintf(buf, "idle\n");
   case 1ul << DPD_STATE_COLLECT:
      return sprintf(buf, "collect\n");
   case 1ul << DPD_STATE_READING:
      return sprintf(buf, "reading\n");
   case 1ul << DPD_STATE_CHECK:
      return sprintf(buf, "check_run\n");
   case 1ul << DPD_STATE_CLR:
      return sprintf(buf, "clr\n");
   }

   dev_warn(dev, "unhandled DPD status: %.8lx\n", (unsigned long)val);
   return sprintf(buf, "unknown\n");
}

static ssize_t
read_dpd_data(struct file *filp, struct kobject *kobj,
              struct bin_attribute *attr, char *buf, loff_t off,
              size_t size)
{
   struct device *dev = container_of(kobj, struct device, kobj);
   unsigned char blob[256];
   size_t i;

   /* DPD data memory consists of 256 bytes (64 * 32-bit words) */
   if (off >= sizeof blob)
      return 0;

   if (size > sizeof blob - off)
      size = sizeof blob - off;
   if (size == 0)
      return 0;

   device_lock(dev);
   if (is_secure(dev)) {
      device_unlock(dev);
      return -EPERM;
   }

   for (i = 0; i < sizeof blob/4; i++) {
      u32 val;

      trng3_indirect_write(dev, TRNG3_IAP_DPD_MEM_PTR, i);
      val = cpu_to_be32(trng3_indirect_read(dev, TRNG3_IAP_DPD_RD_DATA));

      memcpy(blob + 4*i, &val, sizeof val);
   }
   device_unlock(dev);

   memcpy(buf, blob+off, size);
   return size;
}

static RING_ATTR(rejects, 0,  0400, show_rejects,   NULL);
static RING_ATTR(rejects, 1,  0400, show_rejects,   NULL);
static RING_ATTR(rejects, 2,  0400, show_rejects,   NULL);
static RING_ATTR(rejects, 3,  0400, show_rejects,   NULL);
static RING_ATTR(rejects, 4,  0400, show_rejects,   NULL);
static RING_ATTR(rejects, 5,  0400, show_rejects,   NULL);

static RING_ATTR(contrib, 0,  0400, show_contrib,   NULL);
static RING_ATTR(contrib, 1,  0400, show_contrib,   NULL);
static RING_ATTR(contrib, 2,  0400, show_contrib,   NULL);
static RING_ATTR(contrib, 3,  0400, show_contrib,   NULL);
static RING_ATTR(contrib, 4,  0400, show_contrib,   NULL);
static RING_ATTR(contrib, 5,  0400, show_contrib,   NULL);

static RING_ATTR(mux,     0,  0400, show_mux,       NULL);
static RING_ATTR(mux,     1,  0400, show_mux,       NULL);
static RING_ATTR(mux,     2,  0400, show_mux,       NULL);
static RING_ATTR(mux,     3,  0400, show_mux,       NULL);
static RING_ATTR(mux,     4,  0400, show_mux,       NULL);
static RING_ATTR(mux,     5,  0400, show_mux,       NULL);

static RING_ATTR_SUFFIX(ring, _enabled, 0, 0600, show_enable, store_enable);
static RING_ATTR_SUFFIX(ring, _enabled, 1, 0600, show_enable, store_enable);
static RING_ATTR_SUFFIX(ring, _enabled, 2, 0600, show_enable, store_enable);
static RING_ATTR_SUFFIX(ring, _enabled, 3, 0600, show_enable, store_enable);
static RING_ATTR_SUFFIX(ring, _enabled, 4, 0600, show_enable, store_enable);
static RING_ATTR_SUFFIX(ring, _enabled, 5, 0600, show_enable, store_enable);

static DEVICE_ATTR(cond_in,   0400, show_cond_in,   NULL);
static DEVICE_ATTR(first_gen, 0400, show_first_gen, NULL);

static DEVICE_ATTR(simult2,   0400, show_simult,    NULL);
static DEVICE_ATTR(simult3,   0400, show_simult,    NULL);
static DEVICE_ATTR(simult4,   0400, show_simult,    NULL);
static DEVICE_ATTR(simult5,   0400, show_simult,    NULL);
static DEVICE_ATTR(simult6,   0400, show_simult,    NULL);

static DEVICE_ATTR(dpd_clr,   0200, NULL, store_dpd_clr);
static DEVICE_ATTR(dpd_force, 0200, NULL, store_dpd_force);
static DEVICE_ATTR(dpd_state, 0400, show_dpd_state, NULL);
static struct bin_attribute bin_attr_dpd_data = {
   .attr = { .name = "dpd_data", .mode = 0400 },
   .read = read_dpd_data,
};

static const struct attribute_group trng3_debug_attr_group = {
   .name = "debug",
   .attrs = (struct attribute *[]) {
      &ring_attr_rejects0.devattr.attr,
      &ring_attr_rejects1.devattr.attr,
      &ring_attr_rejects2.devattr.attr,
      &ring_attr_rejects3.devattr.attr,
      &ring_attr_rejects4.devattr.attr,
      &ring_attr_rejects5.devattr.attr,

      &ring_attr_contrib0.devattr.attr,
      &ring_attr_contrib1.devattr.attr,
      &ring_attr_contrib2.devattr.attr,
      &ring_attr_contrib3.devattr.attr,
      &ring_attr_contrib4.devattr.attr,
      &ring_attr_contrib5.devattr.attr,

      &ring_attr_mux0.devattr.attr,
      &ring_attr_mux1.devattr.attr,
      &ring_attr_mux2.devattr.attr,
      &ring_attr_mux3.devattr.attr,
      &ring_attr_mux4.devattr.attr,
      &ring_attr_mux5.devattr.attr,

      &ring_attr_ring0_enabled.devattr.attr,
      &ring_attr_ring1_enabled.devattr.attr,
      &ring_attr_ring2_enabled.devattr.attr,
      &ring_attr_ring3_enabled.devattr.attr,
      &ring_attr_ring4_enabled.devattr.attr,
      &ring_attr_ring5_enabled.devattr.attr,

      &dev_attr_cond_in.attr,
      &dev_attr_first_gen.attr,

      &dev_attr_simult2.attr,
      &dev_attr_simult3.attr,
      &dev_attr_simult4.attr,
      &dev_attr_simult5.attr,
      &dev_attr_simult6.attr,

      &dev_attr_dpd_clr.attr,
      &dev_attr_dpd_force.attr,
      &dev_attr_dpd_state.attr,
      NULL,
   },
};

/*
 * Hack to add binary files to a sysfs group since there is no public API to
 * do so.  Inspired by sysfs_add_file_to_group.  We generate a fake kobject
 * if necessary, since internally sysfs_create_bin_file only cares about
 * the sd member in the struct, and we can find out what *that* should
 * be using the public APIs.
 */
static struct kobject *fake_group_kobj(struct kobject *parent,
                                       const char *group, struct kobject *out)
{
   if (!group) {
      sysfs_get(parent->sd);
      return parent;
   }

   out->sd = sysfs_get_dirent(parent->sd, NULL, group);
   if (!out->sd)
      return NULL;

   return out;
}

static int add_bin_file_to_group(struct kobject *kobj,
                                 const struct bin_attribute *attr,
                                 const char *group)
{
   struct kobject fake_kobj, *subdir;
   int rc;

   subdir = fake_group_kobj(kobj, group, &fake_kobj);
   if (!subdir)
      return -ENOENT;

   rc = sysfs_create_bin_file(subdir, attr);
   sysfs_put(subdir->sd);

   return rc;
}

static void remove_bin_file_from_group(struct kobject *kobj,
                                       const struct bin_attribute *attr,
                                       const char *group)
{
   struct kobject fake_kobj, *subdir;

   subdir = fake_group_kobj(kobj, group, &fake_kobj);
   if (!subdir)
      return;

   sysfs_remove_bin_file(subdir, attr);
   sysfs_put(subdir->sd);
}

int trng3_create_debug_attrs(struct device *dev)
{
   struct trng3_priv *priv = dev_get_drvdata(dev);
   int rc;

   if (priv->trng3.diag_level == 0)
      return 0;

   rc = sysfs_create_group(&dev->kobj, &trng3_debug_attr_group);
   if (rc < 0)
      return rc;

   rc = add_bin_file_to_group(&dev->kobj, &bin_attr_dpd_data,
                              trng3_debug_attr_group.name);
   if (rc < 0) {
      sysfs_remove_group(&dev->kobj, &trng3_debug_attr_group);
      return rc;
   }

   return 0;
}

void trng3_remove_debug_attrs(struct device *dev)
{
   struct trng3_priv *priv = dev_get_drvdata(dev);

   if (priv->trng3.diag_level == 0)
      return;

   remove_bin_file_from_group(&dev->kobj, &bin_attr_dpd_data,
                              trng3_debug_attr_group.name);

   sysfs_remove_group(&dev->kobj, &trng3_debug_attr_group);
}
