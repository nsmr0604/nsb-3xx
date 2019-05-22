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

#include "elptrng.h"
#include "elptrng_driver.h"
#include "test_elptrng.h"
#include "crypto/elpcrypto_api.h"

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
	const char *algo = crypto_tfm_alg_driver_name(crypto_rng_tfm(tfm));
	int err = 0, i = 0;
	char result1[TRNG_DATA_SIZE_BYTES];
   char result2[TRNG_DATA_SIZE_BYTES];
   char seed[TRNG_NONCE_SIZE_BYTES];

   /* test 1 */   
   printk("Test 1: Should be able to read random data\n");

   for (i = 0; i < TRNG_NONCE_SIZE_BYTES; i++) {
      seed[i] = template[0].seed_value[i];
   }
   err = crypto_rng_reset(tfm, seed, TRNG_NONCE_SIZE_BYTES);
   if (err != 0) {
      printk(KERN_ERR "ELPTRNG test 1 failure: reseed: %s (requested %d, got %d)\n", algo, 0, err);
		goto out;
	}

   err = crypto_rng_get_bytes(tfm, result1, TRNG_DATA_SIZE_BYTES);
   if (err != TRNG_DATA_SIZE_BYTES) {
      printk(KERN_ERR "ELPTRNG test 1 failure: random: %s (requested %d, got %d)\n", algo, TRNG_DATA_SIZE_BYTES, err);
		goto out;
	}

   printk (" [PASSED]\n");

   /* test 2 */   
   printk("Test 2: Given different initial nonce seeds the rng data is different\n");

   for (i = 0; i < TRNG_NONCE_SIZE_BYTES; i++) {
      seed[i] = template[0].seed_value[i];
   }
   err = crypto_rng_reset(tfm, seed, TRNG_NONCE_SIZE_BYTES);
   if (err != 0) {
      printk(KERN_ERR "ELPTRNG test 2 failure: reseed: %s (requested %d, got %d)\n", algo, 0, err);
		goto out;
	}
   err = crypto_rng_get_bytes(tfm, result1, TRNG_DATA_SIZE_BYTES);
   if (err != TRNG_DATA_SIZE_BYTES) {
      printk(KERN_ERR "ELPTRNG test 2 failure: %s (requested %d, got %d)\n", algo, TRNG_DATA_SIZE_BYTES, err);
		goto out;
	}

   for (i = 0; i < TRNG_NONCE_SIZE_BYTES; i++) {
      seed[i] = 0xff & (template[0].seed_value[i] + 0x01);
   }
   err = crypto_rng_reset(tfm, seed, TRNG_NONCE_SIZE_BYTES);
   if (err != 0) {
      printk(KERN_ERR "ELPTRNG test 2 failure: reseed: %s (requested %d, got %d)\n", algo, 0, err);
		goto out;
	}
   err = crypto_rng_get_bytes(tfm, result2, TRNG_DATA_SIZE_BYTES);
   if (err != TRNG_DATA_SIZE_BYTES) {
      printk(KERN_ERR "ELPTRNG test 2 failure: %s (requested %d, got %d)\n", algo, TRNG_DATA_SIZE_BYTES, err);
		goto out;
	}

   /* Compare the data buffers */
   if (memcmp (result1, result2, TRNG_DATA_SIZE_BYTES) != 0) {
      printk (" [PASSED]\n");
   } else {
      printk (" [FAILED]\n");
      goto out;
   }
   printk ("\n");

   /* test 3 */   
   printk("Test 3: Given the same initial nonce seed the rng data is the same\n");
   for (i = 0; i < TRNG_NONCE_SIZE_BYTES; i++) {
      seed[i] = template[0].seed_value[i];
   }
   err = crypto_rng_reset(tfm, seed, TRNG_NONCE_SIZE_BYTES);
   if (err != 0) {
      printk(KERN_ERR "ELPTRNG test 3 failure: reseed: %s (requested %d, got %d)\n", algo, 0, err);
		goto out;
	}
   err = crypto_rng_get_bytes(tfm, result1, TRNG_DATA_SIZE_BYTES);
   if (err != TRNG_DATA_SIZE_BYTES) {
      printk(KERN_ERR "ELPTRNG test 3 failure: %s (requested %d, got %d)\n", algo, TRNG_DATA_SIZE_BYTES, err);
		goto out;
	}
   
   err = crypto_rng_reset(tfm, seed, TRNG_NONCE_SIZE_BYTES);
   if (err != 0) {
      printk(KERN_ERR "ELPTRNG test 3 failure: reseed: %s (requested %d, got %d)\n", algo, 0, err);
		goto out;
	}
   err = crypto_rng_get_bytes(tfm, result2, TRNG_DATA_SIZE_BYTES);
   if (err != TRNG_DATA_SIZE_BYTES) {
      printk(KERN_ERR "ELPTRNG test 3 failure: %s (requested %d, got %d)\n", algo, TRNG_DATA_SIZE_BYTES, err);
		goto out;
	}
	err = 0;

   /* Compare the data buffers */
   if (memcmp (result1, result2, TRNG_DATA_SIZE_BYTES) == 0) {
      printk (" [PASSED]\n");
   } else {
      printk (" [FAILED]\n");

      /*
      printk("Result 1: ");
      for (i = 0; i < TRNG_DATA_SIZE_BYTES; i++) {         
         printk("0x%x ", (0xff & result1[i]));
      }

      printk("\nResult 2: ");
      for (i = 0; i < TRNG_DATA_SIZE_BYTES; i++) {         
         printk("0x%x ", (0xff & result2[i]));
      }
      */

      goto out;
   }
   printk ("\n");

out:
	return err;
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

static uint32_t cv_seed_data[8] = {
  0x56789ABC,
  0x6789ABCD,
  0x789ABCDE,
  0x89ABCDEF,
  0x12345678,
  0x23456789,
  0x3456789A,
  0x456789AB
};

static uint32_t cv_rng_out[4] = {
  0xB9076EF0,
  0xA126CAB,
  0xF1B20E14,
  0xD202B674
};  

static int common_vector(void)
{
   trng_hw *hw;
   int32_t err;
   uint32_t out[4];
   
      
   hw = trng_get_device();
   err = trng_reseed_nonce(hw, cv_seed_data, 0);
   if (err < 0) {
      printk("trng_common_vector::Failed to seed\n");
      return err;
   }
   err = trng_rand(hw, (uint8_t *)out, 16, 0);
   if (err != 0) {
      printk("trng_common_vector::Failed to rand\n");
      return err;
   }
   err = 0;
   if (memcmp(out, cv_rng_out, 16)) {
      printk("trng_common_vector::Failed memcmp\n");
      return -1;
   }
   printk("trng_common_vector::PASSED\n");
   return 0;
}

static int __init alg_test_start(void)
{
   int err; 
   ELPHW_PRINT("alg_test_start\n");

   err = alg_test(ELPCRYPTO_TRNG_NAME, ELPCRYPTO_TRNG_NAME, CRYPTO_ALG_TYPE_RNG, 0);
   if (err) return err;
   
   err = common_vector();
   return err;
}

static void __exit alg_test_end(void)
{
   ELPHW_PRINT("alg_test_end\n");
}


module_init(alg_test_start);
module_exit(alg_test_end);

MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("Elliptic Technologies Inc.");
