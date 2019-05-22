/*
 * Algorithm testing framework and tests.
 *
 * Copyright (c) 2002 James Morris <jmorris@intercode.com.au>
 * Copyright (c) 2002 Jean-Francois Dive <jef@linuxbe.org>
 * Copyright (c) 2007 Nokia Siemens Networks
 * Copyright (c) 2008 Herbert Xu <herbert@gondor.apana.org.au>
 *
 * Updated RFC4106 AES-GCM testing. Some test vectors were taken from
 * http://csrc.nist.gov/groups/ST/toolkit/BCM/documents/proposedmodes/
 * gcm/gcm-test-vectors.tar.gz
 *     Authors: Aidan O'Mahony (aidan.o.mahony@intel.com)
 *              Adrian Hoban <adrian.hoban@intel.com>
 *              Gabriele Paoloni <gabriele.paoloni@intel.com>
 *              Tadeusz Struk (tadeusz.struk@intel.com)
 *     Copyright (c) 2010, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 */
#ifndef _CRYPTO_TESTMGR_H
#define _CRYPTO_TESTMGR_H

#include <linux/netlink.h>
#include <linux/zlib.h>

#include <crypto/compress.h>

#define MAX_DIGEST_SIZE		64
#define MAX_TAP			8

#define MAX_KEYLEN		56
#define MAX_IVLEN		32

#define ELPTRNG_TEST_VECTORS	1

struct elptrng_testvec {
   unsigned int loops;
   unsigned char seed_value[32];
};

static struct elptrng_testvec elptrng_testvec[] = {
	{	
		.loops	= 1,
      .seed_value = {0x00, 0x34, 0xc2, 0x45, 0xd3, 0x56, 0xc1, 0x78, 
                     0xe9, 0x21, 0x38, 0xb4, 0x81, 0x31, 0x88, 0x62,
                     0x2e, 0x73, 0x90, 0x1f, 0x07, 0x2a, 0x52, 0x81, 
                     0x0a, 0x4a, 0xed, 0x4d, 0xb2, 0xff, 0xef, 0xbf},
	}
};

/*
 * MD4 test vectors from RFC1320
 */
#define MD4_TEST_VECTORS	7

#endif	/* _CRYPTO_TESTMGR_H */
