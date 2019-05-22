#ifndef __EVERIFY_H__
#define __EVERIFY_H__

int do_g2_ecdsa_verify(const void *plain, int plainsz,
		       const unsigned char *sig, int sigsz,
		       const unsigned char *pubk, int pubksz);

int do_g2_ecdsa_verify_by_otp(const void *plain, int plainsz,
		       const unsigned char *sig, int sigsz);

#endif //__EVERIFY_H__
