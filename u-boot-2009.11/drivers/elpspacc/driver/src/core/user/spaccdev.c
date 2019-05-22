/*
 * Copyright (c) 2013 Elliptic Technologies Inc.
 *
 */
#include "elpspaccusr.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int spacc_dev_open(struct elp_spacc_usr *io,
                   int cipher_mode, int hash_mode, int encrypt, int icvmode, int icv_len, int aad_copy,
                   unsigned char *ckey, int ckeylen, unsigned char *civ, int civlen,
                   unsigned char *hkey, int hkeylen, unsigned char *hiv, int hivlen)
{
   memset(io, 0, sizeof *io);

   io->io.partial     = ELP_SPACC_USR_MSG_BEGIN | ELP_SPACC_USR_MSG_END;
   io->io.cipher_mode = cipher_mode;
   io->io.hash_mode   = hash_mode;
   io->io.aad_copy    = aad_copy;
   io->io.icv_mode    = icvmode;
   io->io.icv_len     = icv_len;
   io->io.encrypt     = encrypt;

   io->io.ckeylen     = ckeylen;
   io->io.civlen      = civlen;
   io->io.hkeylen     = hkeylen;
   io->io.hivlen      = hivlen;

   if (ckeylen > sizeof(io->io.ckey) ||
       civlen  > sizeof(io->io.civ)  ||
       hkeylen > sizeof(io->io.hkey) ||
       hivlen  > sizeof(io->io.hiv)) {
      return -1;
   }

   if (!io->io.icv_len) {
      switch (cipher_mode) {
         case CRYPTO_MODE_AES_CCM:
         case CRYPTO_MODE_AES_GCM: io->io.icv_len = 16; break;
      }
      if (!io->io.icv_len) {
         switch (hash_mode) {
            case CRYPTO_MODE_SSLMAC_SHA1:
            case CRYPTO_MODE_HASH_SHA1:
            case CRYPTO_MODE_HMAC_SHA1:        io->io.icv_len =  20; break;
            case CRYPTO_MODE_SSLMAC_MD5:
            case CRYPTO_MODE_HASH_MD5:
            case CRYPTO_MODE_HMAC_MD5:         io->io.icv_len =  16; break;
            case CRYPTO_MODE_HASH_SHA224:
            case CRYPTO_MODE_HMAC_SHA224:      io->io.icv_len =  28; break;
            case CRYPTO_MODE_HASH_SHA256:
            case CRYPTO_MODE_HMAC_SHA256:      io->io.icv_len =  32; break;
            case CRYPTO_MODE_HASH_SHA384:
            case CRYPTO_MODE_HMAC_SHA384:      io->io.icv_len =  48; break;
            case CRYPTO_MODE_HASH_SHA512:
            case CRYPTO_MODE_HMAC_SHA512:      io->io.icv_len =  64; break;
            case CRYPTO_MODE_HASH_SHA512_224:
            case CRYPTO_MODE_HMAC_SHA512_224:  io->io.icv_len =  28; break;
            case CRYPTO_MODE_HASH_SHA512_256:
            case CRYPTO_MODE_HMAC_SHA512_256:  io->io.icv_len =  32; break;
            case CRYPTO_MODE_MAC_XCBC:
            case CRYPTO_MODE_MAC_CMAC:
            case CRYPTO_MODE_MAC_KASUMI_F9:    io->io.icv_len =  16; break;
            case CRYPTO_MODE_HASH_CRC32:
            case CRYPTO_MODE_MAC_SNOW3G_UIA2:
            case CRYPTO_MODE_MAC_ZUC_UIA3:     io->io.icv_len =  4; break;
            case CRYPTO_MODE_NULL:             io->io.icv_len =  0; break;
         }
      }
   }

   // AES-F8 mode has a SALTKEY prepended to the base key so we double
   // the key size internally, note we pass the original ckeylen to the ioctl
   if (cipher_mode == CRYPTO_MODE_AES_F8) {
      ckeylen <<= 1;
   }

   memcpy(io->io.ckey, ckey, ckeylen);
   memcpy(io->io.civ,  civ,  civlen);
   memcpy(io->io.hkey, hkey, hkeylen);
   memcpy(io->io.hiv,  hiv,  hivlen);

   io->fd = open("/dev/spaccusr", O_RDWR);
   if (io->fd < 0) {
      return -1;
   }

   if (ioctl(io->fd, ELP_SPACC_USR_INIT, &io->io) < 0) {
      close(io->fd);
      return -1;
   }

   // if we are in decrypt mode with AES-* we'll run one job to make sure the key is expanded and then one more 0 byte job to reset the IV
   // this is required so we can share the handle with other threads
   if ((cipher_mode == CRYPTO_MODE_AES_CBC ||
       cipher_mode == CRYPTO_MODE_AES_CS1 ||
       cipher_mode == CRYPTO_MODE_AES_CS2 ||
       cipher_mode == CRYPTO_MODE_AES_CS3 ||
       cipher_mode == CRYPTO_MODE_AES_F8  ||
       cipher_mode == CRYPTO_MODE_AES_XTS ||
       cipher_mode == CRYPTO_MODE_AES_CFB) && !encrypt) {
      unsigned char tmp[160];
      int err;
      err = spacc_dev_process(io, NULL, -1, 0, 0, 0, 0, tmp, 32, tmp, sizeof tmp); // cannot encrypt 16 bytes with AES-CBC-CSx
      // trap errors but ignore ICV failures (expected) since we're not decrypting valid data we just want the AES engine to fire
      if (err < 0 && io->io.err != -16) {
         close(io->fd);
         return err;
      }
      err = spacc_dev_process(io, civ, -1,  0, 0, 0, 0, tmp,  0, tmp, sizeof tmp);
      if (err < 0) {
         close(io->fd);
         return err;
      }
      // clear tmp so we're not leaking potentially known ciphertext
      memset(tmp, 0, sizeof tmp);
   }

   return 0;
}

// register a open connection for use by other threads (using the same SPAcc handle)
// we use /dev/urandom to read a 16 byte random seed so other threads cannot easily
// attach themselves to us
int spacc_dev_register(struct elp_spacc_usr *io)
{
   int fd, x;
   fd = open("/dev/urandom", O_RDWR);
   if (fd < 0) {
      return fd;
   }
   x = read(fd, io->io.state_key, 16);
   close(fd);
   if (x < 16) {
      return -1;
   }
   return ioctl(io->fd, ELP_SPACC_USR_REGISTER, &io->io);
}

int spacc_dev_open_bind(struct elp_spacc_usr *master, struct elp_spacc_usr *new)
{
   int err;
   memcpy(new, master, sizeof *new);
   new->fd = open("/dev/spaccusr", O_RDWR);
   if (new->fd < 0) {
      memset(new, 0, sizeof *new);
      return -1;
   }
   err = ioctl(new->fd, ELP_SPACC_USR_BIND, &new->io);
   if (err < 0) {
      close(new->fd);
      memset(new, 0, sizeof *new);
   }
   return err;
}

// used for single buffer jobs
int spacc_dev_process(
   struct elp_spacc_usr *io,
   unsigned char *new_iv,
   int            iv_offset,
   int            pre_aad,
   int            post_aad,
   int            src_offset,
   int            dst_offset,
   unsigned char *src, unsigned long src_len,
   unsigned char *dst, unsigned long dst_len)
{
   int hint;

   // src DDT
      io->io.src[0].ptr   = src;
      io->io.src[0].len   = src_len + src_offset + (io->io.encrypt?0:io->io.icv_len);  // the input mapping includes the offset + ICV (only in decrypt)
      // account for IV import if it's past the end of data
      if (iv_offset >= ((int)src_len + src_offset + (io->io.encrypt?0:io->io.icv_len))) {
         io->io.src[0].len = src_offset + iv_offset + io->io.civlen + (io->io.encrypt?0:io->io.icv_len);
      }
      io->io.src[1].ptr   = NULL;
      io->io.srclen       = src_len;
   // DST ddt
      io->io.dst[0].ptr   = dst;
      io->io.dst[0].len   = dst_len + dst_offset;
      io->io.dst[1].ptr   = NULL;
      io->io.dstlen       = dst_len;


   if (src == dst) {
      if (io->io.src[0].len > io->io.dst[0].len) {
         hint = SPACC_MAP_HINT_USESRC;
      } else {
         hint = SPACC_MAP_HINT_USEDST;
      }
   } else {
      hint = SPACC_MAP_HINT_NOLAP;
   }

   return spacc_dev_process_multi(io, new_iv, iv_offset, pre_aad, post_aad, src_offset, dst_offset, hint);
}

int spacc_dev_process_multi(
   struct elp_spacc_usr *io,
   unsigned char *new_iv,
   int            iv_offset,
   int            pre_aad,
   int            post_aad,
   int            src_offset,
   int            dst_offset,
   int            map_hint)
{
   int err, x;
   if (new_iv) {
      memcpy(io->io.civ, new_iv, io->io.civlen);
   }
   io->io.ivoffset     = iv_offset;
   io->io.pre_aad_len  = pre_aad;
   io->io.post_aad_len = post_aad;
   io->io.src_offset   = src_offset;
   io->io.dst_offset   = dst_offset;
   io->io.map_hint     = map_hint;

   if (io->io.srclen == -1) {
      for (io->io.srclen = x = 0; x < ELP_SPACC_USR_MAX_DDT && io->io.src[x].ptr != NULL; ++x) {
         io->io.srclen += io->io.src[x].len;
      }
   }
   if (io->io.dstlen == -1) {
      for (io->io.dstlen = x = 0; x < ELP_SPACC_USR_MAX_DDT && io->io.dst[x].ptr != NULL; ++x) {
         io->io.dstlen += io->io.dst[x].len;
      }
   }

   err = ioctl(io->fd, new_iv == NULL ? ELP_SPACC_USR_PROCESS : ELP_SPACC_USR_PROCESS_IV, &io->io);

   if (err >= 0) {
      return err;
   } else {
      return -1;
   }
}

int spacc_dev_close(struct elp_spacc_usr *io)
{
   close(io->fd);
   memset(io, 0, sizeof *io);
   return 0;
}

int spacc_dev_features(struct elp_spacc_features *features)
{
   int fd, err;
   fd = open("/dev/spaccusr", O_RDWR);
   if (fd < 0) {
      return -1;
   }
   err = ioctl(fd, ELP_SPACC_USR_FEATURES, features);
   close(fd);
   return err;
}

int spacc_dev_isenabled(struct elp_spacc_features *features, int mode, int keysize)
{
   int x, y;
   static const int keysizes[2][6] = {
      { 5, 8, 16, 24, 32, 0 },     // cipher key sizes
      { 16, 20, 24, 32, 64, 128 },  // hash key sizes
   };

   if (mode < 0 || mode > CRYPTO_MODE_LAST) {
      return 0;
   }

   // NULL mode always supported
   if (mode == CRYPTO_MODE_NULL) {
      return 1;
   }

   if (features->modes[mode] & 128) {
      for (x = 0; x < 6; x++) {
         if (keysizes[1][x] >= keysize && ((1<<x)&features->modes[mode])) {
            return 1;
         }
      }
      return 0;
   } else {
      for (x = 0; x < 6; x++) {
         if (keysizes[0][x] == keysize) {
            if (features->modes[mode] & (1<<x)) {
               return 1;
            } else {
               return 0;
            }
         }
      }
   }
   return 0;
}
