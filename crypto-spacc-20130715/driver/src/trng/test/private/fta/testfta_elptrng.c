/*
 * Algorithm testing framework and tests.
 *
 * Copyright (c) 2002 James Morris <jmorris@intercode.com.au>
 * Copyright (c) 2002 Jean-Francois Dive <jef@linuxbe.org>
 * Copyright (c) 2007 Nokia Siemens Networks
 * Copyright (c) 2008 Herbert Xu <herbert@gondor.apana.org.au>
 *
 * Updated RFC4106 AES-GCM testing.
 *    Authors: Aidan O'Mahony (aidan.o.mahony@intel.com)
 *             Adrian Hoban <adrian.hoban@intel.com>
 *             Gabriele Paoloni <gabriele.paoloni@intel.com>
 *             Tadeusz Struk (tadeusz.struk@intel.com)
 *    Copyright (c) 2010, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 */

#include <linux/err.h>
#include <linux/module.h>
#include <linux/string.h>
#include <crypto/rng.h>
#include <linux/hw_random.h>

#include "elptrng.h"
#include "elptrng_driver.h"
#include "testfta_elptrng.h"
#include "crypto/elpcrypto_api.h"

extern int elptrng_hwrng_driver_read(struct hwrng *rng, void *buf, size_t max, bool wait);
extern int elptrng_crypto_driver_read(struct crypto_rng *tfm, u8 *rdata, unsigned int dlen);
extern int elptrng_crypto_driver_reset(struct crypto_rng *tfm, u8 *seed, unsigned int slen);

static int fips_enabled = 1;

/*
 * Need slab memory for testing (size in number of pages).
 */
#define XBUFSIZE	8


/*
* Used by test_cipher()
*/
#define ENCRYPT 1
#define DECRYPT 0

struct elptrng_test_suite {
	struct elptrng_testvec *vecs;
	unsigned int count;
};

struct alg_test_desc {
	const char *alg;
	int (*test)(const struct alg_test_desc *desc, const char *driver, unsigned int type, unsigned int mask);
	int fips_allowed;	/* set if alg is allowed in fips mode */

	union {	
		struct elptrng_test_suite elptrng;
	} suite;
};

static int test_elptrng(struct crypto_rng *tfm, struct elptrng_testvec *template, unsigned int tcount)
{
	int err = 0;
	char result1[TRNG_DATA_SIZE_BYTES];
   char seed[TRNG_NONCE_SIZE_BYTES];
   struct hwrng rng;
   int pass = 0, fail = 0, numtests=0;

   printk("Test: FTA Crypto API\n");
  
   printk("--- elptrng_crypto_driver_reset ---\n");

   printk("Test: RNG is null ... ");
   err = elptrng_crypto_driver_reset(0, seed, TRNG_NONCE_SIZE_BYTES);
   if (err != -1) {
      printk(KERN_ERR "FAILED\n");
      fail++;
   } else {
      printk(KERN_ERR "PASSED\n");
      pass++;
   }
   numtests++;

   printk("Test: Seed is null ... ");
   err = elptrng_crypto_driver_reset(tfm, 0, TRNG_NONCE_SIZE_BYTES);
   if (err != -1) {
      printk(KERN_ERR "FAILED\n");
      fail++;
   } else {
      printk(KERN_ERR "PASSED\n");
      pass++;
   }
   numtests++;

   printk("Test: Incorrect TRNG NONCE size (+) ... ");
   err = elptrng_crypto_driver_reset(tfm, seed, TRNG_NONCE_SIZE_BYTES+1);
   if (err != 0) {
      printk(KERN_ERR "FAILED\n");
      fail++;
   } else {
      printk(KERN_ERR "PASSED\n");
      pass++;
   }
   numtests++;

   printk("Test: Incorrect TRNG NONCE size (-) ... ");
   err = elptrng_crypto_driver_reset(tfm, seed, TRNG_NONCE_SIZE_BYTES-1);
   if (err != 0) {
      printk(KERN_ERR "FAILED\n");
      fail++;
   } else {
      printk(KERN_ERR "PASSED\n");
      pass++;
   }
   numtests++;

   printk("--- elptrng_crypto_driver_read ---\n");

   printk("Test: RNG is null ... \n");
   err = elptrng_crypto_driver_read(0, result1, TRNG_DATA_SIZE_BYTES);
   if (err != -1) {
      printk(KERN_ERR "FAILED\n");
      fail++;
   } else {
      printk(KERN_ERR "PASSED\n");
      pass++;
   }
   numtests++;

   printk("Test: result1 is null ... ");
   err = elptrng_crypto_driver_read(tfm, 0, TRNG_DATA_SIZE_BYTES);
   if (err != -1) {
      printk(KERN_ERR "FAILED\n");
      fail++;
   } else {
      printk(KERN_ERR "PASSED\n");
      pass++;
   }
   numtests++;

   printk("Test: Incorrect TRNG DATA (+)... ");
   err = elptrng_crypto_driver_read(tfm, result1, TRNG_DATA_SIZE_BYTES+1);
   if (err != TRNG_DATA_SIZE_BYTES) {
      printk(KERN_ERR "FAILED\n");
      fail++;
   } else {
      printk(KERN_ERR "PASSED\n");
      pass++;
   }
   numtests++;

   printk("Test: Incorrect TRNG DATA (0)... ");
   err = elptrng_crypto_driver_read(tfm, result1, 0);
   if (err != -1) {
      printk(KERN_ERR "FAILED\n");
      fail++;
   } else {
      printk(KERN_ERR "PASSED\n");
      pass++;
   }
   numtests++;

   printk("Test: FTA HW Random API\n");

   printk("--- elptrng_hwrng_driver_read ---\n");

   printk("Test: RNG is null... \n");
   err = elptrng_hwrng_driver_read(0, result1, TRNG_DATA_SIZE_BYTES, 0);
   if (err != -1) {
      printk(KERN_ERR "FAILED\n");
      fail++;
   } else {
      printk(KERN_ERR "PASSED\n");
      pass++;
   }
   numtests++;

   printk("Test: result1 is null... ");
   err = elptrng_hwrng_driver_read(&rng, 0, TRNG_DATA_SIZE_BYTES, 0);
   if (err != -1) {
      printk(KERN_ERR "FAILED\n");
      fail++;
   } else {
      printk(KERN_ERR "PASSED\n");
      pass++;
   }
   numtests++;

   printk("Test: Incorrect TRNG DATA (0)... ");
   err = elptrng_hwrng_driver_read(&rng, result1, 0, 0);
   if (err != -1) {
      printk(KERN_ERR "FAILED\n");
      fail++;
   } else {
      printk(KERN_ERR "PASSED\n");
      pass++;
   }
   numtests++;

   printk("Number of tests: %d Pass = %d Fail = %d\n", numtests, pass, fail);

   return 0;
}

static int alg_test_elptrng(const struct alg_test_desc *desc, const char *driver, unsigned int type, unsigned int mask)
{
	struct crypto_rng *rng;
	int err;   

	rng = crypto_alloc_rng(driver, type, mask);
	if (IS_ERR(rng)) {
		printk(KERN_ERR "alg: elptrng: Failed to load transform for %s: "
		       "%ld\n", driver, PTR_ERR(rng));
		return PTR_ERR(rng);
	}

	err = test_elptrng(rng, desc->suite.elptrng.vecs, desc->suite.elptrng.count);

	crypto_free_rng(rng);

	return err;
}


/* Please keep this list sorted by algorithm name. */
static const struct alg_test_desc alg_test_descs[] = {
	 {
		.alg = ELPCRYPTO_TRNG_NAME,
		.test = alg_test_elptrng,
		.fips_allowed = 1,
		.suite = {
			.elptrng = {
				.vecs = elptrng_testvec,
				.count = ELPTRNG_TEST_VECTORS
			}
		}
    }
};

static int alg_find_test(const char *alg)
{
	int start = 0;
	int end = ARRAY_SIZE(alg_test_descs);

	while (start < end) {
		int i = (start + end) / 2;
		int diff = strcmp(alg_test_descs[i].alg, alg);

		if (diff > 0) {
			end = i;
			continue;
		}

		if (diff < 0) {
			start = i + 1;
			continue;
		}

		return i;
	}

	return -1;
}

int alg_test(const char *driver, const char *alg, unsigned int type, unsigned int mask)
{
	int i;
	int j;
	int rc;
	
	i = alg_find_test(alg);
	j = alg_find_test(driver);
	if (i < 0 && j < 0)
		goto notest;

	if (fips_enabled && ((i >= 0 && !alg_test_descs[i].fips_allowed) ||
			     (j >= 0 && !alg_test_descs[j].fips_allowed)))
		goto non_fips_alg;

	rc = 0;
   if (i >= 0) {
		rc |= alg_test_descs[i].test(alg_test_descs + i, driver,
					     type, mask);
      return rc;
   }
   if (j >= 0) {
		rc |= alg_test_descs[j].test(alg_test_descs + j, driver,
					     type, mask);
      return rc;
   }

   if (fips_enabled && rc)
      printk("PANIC: %s: %s alg self test failed in fips mode!\n", driver, alg);

	if (fips_enabled && !rc)
		printk(KERN_INFO "alg: self-tests for %s (%s) passed\n",
		       driver, alg);

	return rc;

notest:
	printk(KERN_INFO "alg: No test for %s (%s)\n", alg, driver);
	return 0;
non_fips_alg:
	return -EINVAL;
}

static int __init alg_test_start(void)
{
   ELPHW_PRINT("ELPTRNG Module FTA test start\n");

   alg_test(ELPCRYPTO_TRNG_NAME, ELPCRYPTO_TRNG_NAME, CRYPTO_ALG_TYPE_RNG, 0);

   return 0;
}

static void __exit alg_test_end(void)
{
   ELPHW_PRINT("ELPTRNG Module FTA test end\n");
}


module_init(alg_test_start);
module_exit(alg_test_end);

MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("Elliptic Technologies Inc.");