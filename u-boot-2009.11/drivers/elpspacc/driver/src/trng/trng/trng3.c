#include "elptrng3.h"
#include "elptrng3_hw.h"

/*
 * Helper to perform a read-modify-write to change a specified register.
 * Any bits in the register which are set in mask are assigned with the
 * corresponding bit in val.  All other bits are unchanged.
 */
static uint32_t elptrng3_rmw(struct elptrng3_state *trng3, unsigned reg,
                                             uint32_t val, uint32_t mask)
{
   uint32_t tmp;

   tmp = elptrng3_readreg(trng3, reg);
   tmp &= ~mask;
   tmp |= (val & mask);
   elptrng3_writereg(trng3, reg, tmp);

   return tmp;
}

static void trng3_mmio_writereg(void *base, unsigned offset, uint32_t val)
{
   uint32_t *regs = base;

   pdu_io_write32(&regs[offset], val);
}

static uint32_t trng3_mmio_readreg(void *base, unsigned offset)
{
   uint32_t *regs = base;

   return pdu_io_read32(&regs[offset]);
}

void elptrng3_setup(struct elptrng3_state *trng3, uint32_t *regbase)
{
   uint32_t tmp;

   if (regbase) {
      trng3->writereg = trng3_mmio_writereg;
      trng3->readreg  = trng3_mmio_readreg;
      trng3->regbase  = regbase;
   }

   /* Put the device into a sane initial state */
   elptrng3_disable_irqs(trng3, -1);
   elptrng3_writereg(trng3, TRNG3_ISTAT, -1);
   elptrng3_rmw(trng3, TRNG3_SMODE, 0, 1ul<<TRNG3_SMODE_NONCE);

   /* Parse out BUILD_ID register */
   tmp = elptrng3_readreg(trng3, TRNG3_BUILD_ID);
   trng3->epn       = (tmp >> TRNG3_BUILD_ID_EPN);
   trng3->epn      &= (1ul << TRNG3_BUILD_ID_EPN_BITS) - 1;
   trng3->stepping  = (tmp >> TRNG3_BUILD_ID_STEPPING);
   trng3->stepping &= (1ul << TRNG3_BUILD_ID_STEPPING_BITS) - 1;

   /* Parse out FEATURES register */
   tmp = elptrng3_readreg(trng3, TRNG3_FEATURES);
   trng3->secure_reset = (tmp >> TRNG3_FEATURES_SECURE_RST) & 1;
   trng3->rings_avail = (tmp >> TRNG3_FEATURES_RAND_SEED) & 1;

   trng3->output_len  = tmp >> TRNG3_FEATURES_RAND_LEN;
   trng3->output_len &= (1ul << TRNG3_FEATURES_RAND_LEN_BITS) - 1;
   trng3->output_len  = 16ul << trng3->output_len;

   trng3->diag_level  = tmp >> TRNG3_FEATURES_DIAG_LEVEL;
   trng3->diag_level &= (1ul << TRNG3_FEATURES_DIAG_LEVEL_BITS) - 1;
}

int elptrng3_reseed(struct elptrng3_state *trng3, const void *nonce)
{
   uint32_t stat, smode, nonce_buf[8];
   size_t i;

   if (nonce) {
      smode = elptrng3_rmw(trng3, TRNG3_SMODE, -1, 1ul<<TRNG3_SMODE_NONCE);

      memcpy(nonce_buf, nonce, sizeof nonce_buf);
      do {
         stat = elptrng3_readreg(trng3, TRNG3_STAT);
      } while (((stat >> TRNG3_STAT_NONCE_MODE) & 1) == 0);

      for (i = 0; i < sizeof nonce_buf / sizeof nonce_buf[0]; i++) {
         elptrng3_writereg(trng3, TRNG3_SEED_BASE+i, nonce_buf[i]);
      }
      elptrng3_writereg(trng3, TRNG3_CTRL, TRNG3_CMD_NONCE_RESEED);
      elptrng3_writereg(trng3, TRNG3_SMODE, smode & ~(1ul<<TRNG3_SMODE_NONCE));
   } else {
      if (!trng3->rings_avail)
         return CRYPTO_MODULE_DISABLED;

      elptrng3_rmw(trng3, TRNG3_SMODE, 0, 1ul<<TRNG3_SMODE_NONCE);
      elptrng3_writereg(trng3, TRNG3_CTRL, TRNG3_CMD_RAND_RESEED);
   }

   return 0;
}

int elptrng3_get_seed(struct elptrng3_state *trng3, void *out)
{
   uint32_t stat, seed_buf[8];
   size_t i;

   stat = elptrng3_readreg(trng3, TRNG3_STAT);
   if (!((stat >> TRNG3_STAT_SEEDED) & 1))
      return CRYPTO_NOT_INITIALIZED;

   for (i = 0; i < sizeof seed_buf / sizeof seed_buf[0]; i++) {
      seed_buf[i] = elptrng3_readreg(trng3, TRNG3_SEED_BASE+i);
   }

   memcpy(out, seed_buf, sizeof seed_buf);
   return 0;
}

void elptrng3_enable_irqs(struct elptrng3_state *trng3, uint32_t irq_mask)
{
   irq_mask &= TRNG3_IRQ_ALL_MASK;

   elptrng3_rmw(trng3, TRNG3_IE, -1, irq_mask);
}

void elptrng3_disable_irqs(struct elptrng3_state *trng3, uint32_t irq_mask)
{
   irq_mask &= TRNG3_IRQ_ALL_MASK;

   elptrng3_rmw(trng3, TRNG3_IE, 0, irq_mask);
}

void elptrng3_set_secure(struct elptrng3_state *trng3, int secure)
{
   if (secure)
      elptrng3_rmw(trng3, TRNG3_SMODE, -1, 1ul << TRNG3_SMODE_SECURE);
   else
      elptrng3_rmw(trng3, TRNG3_SMODE, 0, 1ul << TRNG3_SMODE_SECURE);
}

int elptrng3_set_max_rejects(struct elptrng3_state *trng3, unsigned rejects)
{
   uint32_t val, mask;

   if (rejects >= (1ul << TRNG3_SMODE_MAX_REJECTS_BITS))
      return CRYPTO_INVALID_ARGUMENT;

   val  = rejects << TRNG3_SMODE_MAX_REJECTS;
   mask = ((1ul << TRNG3_SMODE_MAX_REJECTS_BITS)-1) << TRNG3_SMODE_MAX_REJECTS;
   elptrng3_rmw(trng3, TRNG3_SMODE, val, mask);

   return 0;
}

int elptrng3_set_output_len(struct elptrng3_state *trng3, unsigned outlen)
{
   switch (outlen) {
   case 16:
      elptrng3_rmw(trng3, TRNG3_MODE, 0, 1ul << TRNG3_MODE_R256);
      return 0;
   case 32:
      if (trng3->output_len < 32)
         return CRYPTO_MODULE_DISABLED;
      elptrng3_rmw(trng3, TRNG3_MODE, -1, 1ul << TRNG3_MODE_R256);
      return 0;
   }

   return CRYPTO_INVALID_ARGUMENT;
}

int elptrng3_set_request_reminder(struct elptrng3_state *trng3,
                                  unsigned long val)
{
   if (val > ULONG_MAX - TRNG3_AUTO_RQST_RESOLUTION)
      return CRYPTO_INVALID_ARGUMENT;

   val = (val + TRNG3_AUTO_RQST_RESOLUTION - 1) / TRNG3_AUTO_RQST_RESOLUTION;
   if (val >= 1ul << TRNG3_AUTO_RQST_RQSTS_BITS)
      return CRYPTO_INVALID_ARGUMENT;

   elptrng3_writereg(trng3, TRNG3_AUTO_RQST, val);
   return 0;
}

int elptrng3_set_age_reminder(struct elptrng3_state *trng3,
                              unsigned long long val)
{
   if (val > ULLONG_MAX - TRNG3_AUTO_AGE_RESOLUTION)
      return CRYPTO_INVALID_ARGUMENT;

   val = (val + TRNG3_AUTO_AGE_RESOLUTION - 1) / TRNG3_AUTO_AGE_RESOLUTION;
   if (val >= 1ul << TRNG3_AUTO_AGE_AGE_BITS)
      return CRYPTO_INVALID_ARGUMENT;

   elptrng3_writereg(trng3, TRNG3_AUTO_AGE, val);
   return 0;
}

int elptrng3_get_random(struct elptrng3_state *trng3, void *out)
{
   uint32_t stat, istat;
   size_t i, outlen;
   u32 rand_buf[8];

   stat = elptrng3_readreg(trng3, TRNG3_STAT);
   if (!((stat >> TRNG3_STAT_SEEDED) & 1))
      return CRYPTO_NOT_INITIALIZED;
   if ((stat >> TRNG3_STAT_GENERATING) & 1)
      return CRYPTO_INPROGRESS;

   istat = elptrng3_readreg(trng3, TRNG3_ISTAT);
   if (!(istat & TRNG3_IRQ_RAND_RDY_MASK))
      return 0;

   outlen = (stat >> TRNG3_STAT_R256) & 1 ? 8 : 4;
   for (i = 0; i < outlen; i++) {
      rand_buf[i] = elptrng3_readreg(trng3, TRNG3_RAND_BASE + i);
   }

   elptrng3_writereg(trng3, TRNG3_ISTAT, TRNG3_IRQ_RAND_RDY_MASK);

   if (out) {
      memcpy(out, rand_buf, i * sizeof rand_buf[0]);
   }

   return i * sizeof rand_buf[0];
}
