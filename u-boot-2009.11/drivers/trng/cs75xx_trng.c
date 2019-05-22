#include <config.h>
#include <common.h>

#include <registers.h>
#include <platform.h>
#include "elptrng.h"

int generate_trng( char *data, unsigned int len)
{
	int rc;
	uint32_t value;
	struct trng_hw hw;

	value = readl(GLOBAL_SCRATCH);
	writel(value | 0x20, GLOBAL_SCRATCH);

	memset(&hw, 0, sizeof(struct trng_hw));
	if (trng_init(&hw, GOLDENGATE_TRNG_BASE,
		      ELPTRNG_IRQ_PIN_DISABLE, ELPTRNG_RESEED) != ELPTRNG_OK) {
		return 1;
	}

	hw.initialized = ELPTRNG_INIT;

	rc = trng_rand(&hw, data, len);
	trng_close(&hw);

	return rc;
}
