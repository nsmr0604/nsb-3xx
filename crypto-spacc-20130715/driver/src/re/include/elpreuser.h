/*
 * Copyright (c) 2013 Elliptic Technologies Inc.
 *
 */
#include "../../pdu/linux/include/elppdu_error.h"

#define RE_MAX_SIZE      (18UL*1024UL)

enum {
   ELP_SPACC_RE_USR_INIT=200,
   ELP_SPACC_RE_USR_REGISTER,
   ELP_SPACC_RE_USR_BIND,
   ELP_SPACC_RE_USR_WRITE_CONTEXT,
   ELP_SPACC_RE_USR_READ_CONTEXT,
   ELP_SPACC_RE_USR_DATA_OP,
};

enum {
   RE_DEV_CCS=0,
   RE_DEV_ALERT,
   RE_DEV_HANDSHAKE,
   RE_DEV_WRITE_MODE,
   RE_DEV_READ_MODE,
};

#ifndef KERNEL
   #ifndef __user
      #define __user
   #endif
#endif

#define ELP_SPACC_RE_USR_MAX_DDT 8  // max userspace segments per src/dst

struct elp_spacc_re_usr_ddt {
   void __user *ptr;
   int          len;
};

#define ELP_SPACC_RE_SRC_DDT(fd, x, addr, len) do { (fd)->io.src[x] = (struct elp_spacc_re_usr_ddt){(addr),(len)}; } while (0);
#define ELP_SPACC_RE_DST_DDT(fd, x, addr, len) do { (fd)->io.dst[x] = (struct elp_spacc_re_usr_ddt){(addr),(len)}; } while (0);
#define ELP_SPACC_RE_SRC_TERM(fd, x) do { (fd)->io.src[x] = (struct elp_spacc_re_usr_ddt){NULL,0}; } while (0);
#define ELP_SPACC_RE_DST_TERM(fd, x) do { (fd)->io.dst[x] = (struct elp_spacc_re_usr_ddt){NULL,0}; } while (0);

#define ELP_SPACC_RE_SRCLEN_SET(fd, x) (fd)->io.srclen = x
#define ELP_SPACC_RE_DSTLEN_SET(fd, x) (fd)->io.dstlen = x

#define ELP_SPACC_RE_SRCLEN_RESET(fd) (fd)->io.srclen = -1
#define ELP_SPACC_RE_DSTLEN_RESET(fd) (fd)->io.dstlen = -1

enum {
   RE_MAP_HINT_TEST=0,  // test internally if there is valid overlap
   RE_MAP_HINT_USESRC,  // there is overlap and use the source mapping (because the destination mapping is contained inside it)
   RE_MAP_HINT_USEDST,  // there is overlap and use the destination mapping
   RE_MAP_HINT_NOLAP    // there is no overlap so don't bother trying to test for it
};

struct elp_spacc_re_ioctl {
   unsigned char
                cmd,          // RE command
                version,      // which version of SSL/TLS
                cipher_mode,  // cipher mode
                hash_mode,    // hash mode
                encrypt,      // 1 to set encrypt mode
                ckeylen,      // cipher key length
                civlen,       // cipher IV length
                hkeylen,      // hash key length
                map_hint;     // see RE_MAP_HINT_*
   int
                err;

   struct elp_spacc_re_usr_ddt
                src[ELP_SPACC_RE_USR_MAX_DDT+1],
                dst[ELP_SPACC_RE_USR_MAX_DDT+1];

   int          srclen, dstlen,              // despite the DDTs above these need to be filled in to make things simpler inside the kernel
                src_offset, dst_offset;      // offsets into buffers

   unsigned char civ[16];
   unsigned char ckey[32], hkey[64], params[2], sequence_number[8];
   unsigned char state_key[16];              // state key used to allow multiple file handle use the same SPAcc handle
}__attribute__((packed));

struct elp_spacc_re_usr {
   int fd;
   struct elp_spacc_re_ioctl io;
};

#ifndef KERNEL

#define RE_USR_ERR(fd) re_dev_error_map((fd)->io.err)


#include <stdint.h>
#include <elprehw.h>

enum {
   RE_DEV_DIR_WRITE=0,
   RE_DEV_DIR_READ
};

int re_dev_open(struct elp_spacc_re_usr *io, int version);
int re_dev_open_bind(struct elp_spacc_re_usr *master, struct elp_spacc_re_usr *new);
int re_dev_register(struct elp_spacc_re_usr *io);

int re_dev_set_context(struct elp_spacc_re_usr *io, int dir,
                   unsigned char *iv,              uint32_t ivlen,
                   unsigned char *key,             uint32_t keylen,
                   unsigned char *mackey,          uint32_t mackeylen,
                   unsigned char *params,          uint32_t paramlength,
                   unsigned char *sequence_number, uint32_t seqlength);

#define re_dev_set_write_context(fd,iv,ivlen,key,keylen,mackey,mackeylen,params,paramlength,sequence_number,seqlength) \
   re_dev_set_context(fd, RE_DEV_DIR_WRITE,iv,ivlen,key,keylen,mackey,mackeylen,params,paramlength,sequence_number,seqlength)

#define re_dev_set_read_context(fd,iv,ivlen,key,keylen,mackey,mackeylen,params,paramlength,sequence_number,seqlength) \
   re_dev_set_context(fd, RE_DEV_DIR_READ,iv,ivlen,key,keylen,mackey,mackeylen,params,paramlength,sequence_number,seqlength)

int re_dev_do_record(struct elp_spacc_re_usr *io,
                     const unsigned char *in,        uint32_t  inlen,
                           unsigned char *out,       uint32_t outlen,
                                     int src_offset,      int  dst_offset,
                                     int cmd);
int re_dev_do_record_multi(struct elp_spacc_re_usr *io, int src_offset, int dst_offset, int hint, int cmd);

int re_dev_close(struct elp_spacc_re_usr *io);
int re_dev_error_map(int k_err);

/* for Elliptic's SPAcc-RE ... */
enum {
   RE_HASH_NULL=0,
   RE_HASH_MD5_128,              // SSLMAC/HMAC MD5-128
   RE_HASH_MD5_80,               // SSLMAC/HMAC MD5-80
   RE_HASH_SHA1_160,
   RE_HASH_SHA1_80,
   RE_HASH_SHA256_256,
   RE_HASH_SHA256_80,
};

enum {
   RE_CIPHER_NULL=0,
   RE_CIPHER_RC4_40,
   RE_CIPHER_RC4_128,
   RE_CIPHER_DES_CBC,
   RE_CIPHER_3DES_CBC,
   RE_CIPHER_AES128_CBC,
   RE_CIPHER_AES256_CBC,
   RE_CIPHER_AES128_GCM,
   RE_CIPHER_AES256_GCM,
};

#define ELP_RE_PARAMS(param, hash, cipher, ar_en) \
   do { params[0] = (ar_en ? 2 : 0); params[1] = (hash<<0)|(cipher<<4); } while(0);

enum {
   RE_VERSION_SSL3_0=0,
   RE_VERSION_TLS1_0,
   RE_VERSION_TLS1_1,
   RE_VERSION_TLS1_2,
   RE_VERSION_DTLS=15
};


#endif

