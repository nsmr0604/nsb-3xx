/*
 * Copyright (c) 2013 Elliptic Technologies Inc.
 */

#define KEP_MAX_SIZE  (1024UL)

#ifndef KERNEL

#include <stdint.h>

// opcodes
enum {
   KEP_SSL3_KEYGEN=0,
   KEP_SSL3_SIGN,
   KEP_TLS_PRF,
   KEP_TLS_SIGN,
   KEP_TLS2_PRF,
   KEP_TLS2_SIGN
};   

// options for commands
#define KEP_OPT_SSL3_SIGN_MD5   0
#define KEP_OPT_SSL3_SIGN_SHA1  1

#define KEP_OPT_TLS_SIGN_CLIENT 0
#define KEP_OPT_TLS_SIGN_SERVER 1

#define KEP_OPT_TLS2_PRF_MD5    (1UL<<1)
#define KEP_OPT_TLS2_PRF_SHA1   (2UL<<1)
#define KEP_OPT_TLS2_PRF_SHA224 (3UL<<1)
#define KEP_OPT_TLS2_PRF_SHA256 (4UL<<1)
#define KEP_OPT_TLS2_PRF_SHA384 (5UL<<1)
#define KEP_OPT_TLS2_PRF_SHA512 (6UL<<1)

#define KEP_OPT_TLS2_SIGN_CLIENT 0
#define KEP_OPT_TLS2_SIGN_SERVER 1
#define KEP_OPT_TLS2_SIGN_MD5    (1UL<<1)
#define KEP_OPT_TLS2_SIGN_SHA1   (2UL<<1)
#define KEP_OPT_TLS2_SIGN_SHA224 (3UL<<1)
#define KEP_OPT_TLS2_SIGN_SHA256 (4UL<<1)
#define KEP_OPT_TLS2_SIGN_SHA384 (5UL<<1)
#define KEP_OPT_TLS2_SIGN_SHA512 (6UL<<1)

int kep_ssl3_keygen(const unsigned char *master_secret,
                    const unsigned char *server_secret,
                    const unsigned char *client_secret,
                          unsigned char *key, uint32_t keylen);
                          
int kep_ssl3_sign(const unsigned char *sign_data, uint32_t sign_len, uint32_t options,
                  const unsigned char *master_secret,
                        unsigned char *dgst, uint32_t dgst_len);
                        
int kep_tls_prf(const unsigned char *label, uint32_t label_len, uint32_t options,
                const unsigned char *master_secret,
                const unsigned char *server_secret, uint32_t server_len,
                      unsigned char *prf, uint32_t prf_len);

int kep_tls_sign(const unsigned char *sign_data, uint32_t sign_len, uint32_t options,
                 const unsigned char *master_secret,
                       unsigned char *dgst, uint32_t dgst_len);


int kep_tls2_prf(const unsigned char *label, uint32_t label_len, uint32_t options,
                 const unsigned char *master_secret, uint32_t master_len,
                 const unsigned char *server_secret, uint32_t server_len,
                       unsigned char *prf, uint32_t prf_len);

int kep_tls2_sign(const unsigned char *sign_data, uint32_t sign_len, uint32_t options,
                  const unsigned char *master_secret, uint32_t master_len,
                        unsigned char *dgst, uint32_t dgst_len);

#endif
