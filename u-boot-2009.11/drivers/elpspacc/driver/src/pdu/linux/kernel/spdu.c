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
#include <linux/version.h>
#if LINUX_VERSION_CODE != KERNEL_VERSION (2,6,36)
#include <linux/export.h>
#endif
#include <linux/platform_device.h>
#include <linux/io-mapping.h>
#include <linux/random.h>
#include "elptrnghw.h"
#include "elppdu.h"

/*
 * Boot the RNG when a nonce reseed is required.  This must be called *BEFORE*
 * the RNG platform device is registered, as it requires exclusive access to
 * the RNG.  We cannot use the TRNG SDK here because (currently) the TRNG
 * driver requires this driver to already be loaded.
 */
void spdu_boot_trng(pdu_info *info, unsigned long spdu_base)
{
   uint8_t __iomem *trng_regs; /* XXX: TRNG headers use byte offsets. */
   uint32_t nonce[8], cfg;

   if (info->pdu_config.is_rng) {
      trng_regs = ioremap(spdu_base + 0x18000, 0x1000);
      if (!trng_regs) {
         pr_warn("failed to seed SPAcc-PDU RNG; some features may not work correctly.\n");
         return;
      }

      cfg = pdu_io_read32(&trng_regs[TRNG_CFG]);
      if (((cfg >> TRNG_CFG_RINGS) & ((1ul << TRNG_CFG_RINGS_BITS)-1)) == 1) {
         pr_info("TRNG has rings, automatic nonce reseed skipped\n");
         goto out_unmap;
      }

      pr_info("TRNG has no rings, performing automatic nonce reseed\n");

      get_random_bytes(nonce, sizeof nonce);

      preempt_disable();
      pdu_io_write32(&trng_regs[TRNG_CTRL], TRNG_NONCE_RESEED);

      pdu_io_write32(&trng_regs[TRNG_DATA+0x0], nonce[0]);
      pdu_io_write32(&trng_regs[TRNG_DATA+0x4], nonce[1]);
      pdu_io_write32(&trng_regs[TRNG_DATA+0x8], nonce[2]);
      pdu_io_write32(&trng_regs[TRNG_DATA+0xc], nonce[3]);
      pdu_io_write32(&trng_regs[TRNG_CTRL], TRNG_NONCE_RESEED
                                            | TRNG_NONCE_RESEED_LD);

      pdu_io_write32(&trng_regs[TRNG_DATA+0x0], nonce[4]);
      pdu_io_write32(&trng_regs[TRNG_DATA+0x4], nonce[5]);
      pdu_io_write32(&trng_regs[TRNG_DATA+0x8], nonce[6]);
      pdu_io_write32(&trng_regs[TRNG_DATA+0xc], nonce[7]);
      pdu_io_write32(&trng_regs[TRNG_CTRL], TRNG_NONCE_RESEED
                                            | TRNG_NONCE_RESEED_LD
                                            | TRNG_NONCE_RESEED_SELECT);

      pdu_io_write32(&trng_regs[TRNG_CTRL], 0);
      mmiowb();
      preempt_enable();

out_unmap:
      iounmap(trng_regs);
   }
}
EXPORT_SYMBOL(spdu_boot_trng);
