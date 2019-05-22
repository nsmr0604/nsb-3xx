#ifndef ELPTRNG3_H_
#define ELPTRNG3_H_

#include "elppdu.h"

#define ELPTRNG3_MAXLEN 32

struct elptrng3_state {
   void     (*writereg)(void *base, unsigned offset, uint32_t val);
   uint32_t (*readreg) (void *base, unsigned offset);

   void *regbase;

   unsigned short epn, stepping, output_len, diag_level;
   unsigned secure_reset:1,
            rings_avail:1;
};

void elptrng3_setup(struct elptrng3_state *trng3, uint32_t *regbase);

/*
 * Enable/disable the IRQ sources specified in irq_mask, which is the
 * bitwise-OR of one or more of the TRNG3_IRQ_xxx_MASK macros in
 * elptrng3_hw.h.
 */
void elptrng3_enable_irqs(struct elptrng3_state *trng3, uint32_t irq_mask);
void elptrng3_disable_irqs(struct elptrng3_state *trng3, uint32_t irq_mask);

/*
 * If secure is non-zero, switch the TRNG into secure mode.  Otherwise,
 * switch the TRNG into promiscuous mode.
 */
void elptrng3_set_secure(struct elptrng3_state *trng3, int secure);

/*
 * Adjust the bit rejection tweak threshold.
 */
int elptrng3_set_max_rejects(struct elptrng3_state *trng3, unsigned rejects);

/*
 * Initiate a reseed of the engine.  If nonce is a null pointer, then a
 * random reseed operation is performed.  Otherwise, nonce should point to
 * a 32-byte buffer which will be used to perform a nonce reseed.
 */
int elptrng3_reseed(struct elptrng3_state *trng3, const void *nonce);

/*
 * Read the last-used seed into a buffer, which must be at least 32 bytes long.
 */
int elptrng3_get_seed(struct elptrng3_state *trng3, void *out);

/*
 * Configure the output length of the engine.  Possible values are 16 and 32
 * bytes.  32-byte mode requires appropriate H/W configuration.
 */
int elptrng3_set_output_len(struct elptrng3_state *trng3, unsigned outlen);

/*
 * Configure the request-based reseed reminder alarm to trigger after the
 * specified number of random numbers generated.
 */
int elptrng3_set_request_reminder(struct elptrng3_state *trng3,
                                  unsigned long requests);

/*
 * Configure the age-based reseed reminder alarm to trigger after the specified
 * number of clock cycles.
 */
int elptrng3_set_age_reminder(struct elptrng3_state *trng3,
                              unsigned long long cycles);

/*
 * Retrieve random data from the engine; out should point to a buffer
 * that is at least 32 bytes long to accomodate all possible output
 * sizes.  On success, returns the number of output bytes written to
 * the buffer (may be either 16 or 32).
 */
int elptrng3_get_random(struct elptrng3_state *trng3, void *out);

/* Helpers to access TRNG registers directly. */
static inline void
elptrng3_writereg(struct elptrng3_state *trng3, unsigned offset, uint32_t val)
{
   trng3->writereg(trng3->regbase, offset, val);
}

static inline uint32_t
elptrng3_readreg(struct elptrng3_state *trng3, unsigned offset)
{
   return trng3->readreg(trng3->regbase, offset);
}

#endif
