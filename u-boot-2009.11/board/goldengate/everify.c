#include <common.h>
#include <command.h>
#include <malloc.h>
#include <serial.h>
#include <nand.h>
#include <mmc.h>

#include "everify.h"
#include <../lib_openssl/ossl-api.h>

OSSLAPI*		osslapi;

#define GOLDENGATE_OTP_BASE		((unsigned char*)0xF5008000)	/* OTP base address*/
#define GOLDENGATE_OTP_SIZE			      0x400	/* OTP size*/
#define	OTP_MAGIC	0x3C

int check_OTP(unsigned char *ptr)
{
	if (ptr[GOLDENGATE_OTP_SIZE - 1] == 0xA5)
		return 1;

	if (ptr[0] == 0x5A)
		return 1;

	if (ptr[GOLDENGATE_OTP_SIZE - 1] == 0 || ptr[0] == 0)
		return 0;

	return 2;
}


static int get_TLV_by_type(unsigned char *otp_addr, unsigned short otp_size,
			    unsigned char etype, unsigned char *buf,
			    unsigned int *length)
{
	unsigned short i, type, len;
	unsigned char *ptrotp, *pdata, *end;

	ptrotp = otp_addr + otp_size - 1;
	end = otp_addr + 1;
	pdata = buf;

	for (;;) {
		if (ptrotp - end < 4)
			return 1;

		if (ptrotp[0] != OTP_MAGIC) {
			ptrotp--;
			continue;
		}

		type = ptrotp[-1];
		if (type < 1 || type > 32) {
			ptrotp--;
			continue;
		}

		len = ptrotp[-2] | (ptrotp[-3] << 8);
		if (len > ptrotp - end) {
			ptrotp--;
			continue;
		}

		ptrotp -= 4;

		if (type != etype) {
			ptrotp -= len;
			continue;
		}

		if (*length < len)
			return 1;

		*length = len;
		for (i = 0; i < len; i++)
			*pdata++ = *ptrotp--;
		return 0;
	}

	return 1;
}


int do_g2_ecdsa_verify(const void *plain, int plainsz,
		       const unsigned char *sig, int sigsz,
		       const unsigned char *pubk, int pubksz)
{
	OSSLAPI *osslapi = ossl_init();

	return osslapi->ecdsa_verify(plain, plainsz, sig, sigsz, pubk, pubksz);
}

int do_g2_ecdsa_verify_by_otp(const void *plain, int plainsz,
		       const unsigned char *sig, int sigsz)
{
	unsigned char key[256];
	unsigned int key_len = sizeof(key);

	if (check_OTP(GOLDENGATE_OTP_BASE) != 1) {
		return 1;
	}

	if (get_TLV_by_type(GOLDENGATE_OTP_BASE, GOLDENGATE_OTP_SIZE, 1, key,
			    &key_len) != 0) {
		return 1;
	}

	if (key_len == 0) {
		return 1;
	}

	return do_g2_ecdsa_verify(plain, plainsz, sig, sigsz, key, key_len);
}
