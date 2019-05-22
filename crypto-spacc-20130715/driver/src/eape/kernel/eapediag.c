/*
 * Copyright (c) 2013 Elliptic Technologies Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/io.h>

#include <linux/fs.h>
#include <linux/dma-mapping.h>
#include <linux/completion.h>
#include <asm/uaccess.h>
#include <asm/param.h>
#include <linux/platform_device.h>
#include "eape.h"
#include "cs75xx.h"

#define MAXHEADERS 16

enum {
   ESP=1,
   AH=2
};

enum {
   INBOUND=1,
   OUTBOUND=2
};

enum {
   eAlgNULL = 0,
   eAlgDES,
   eAlg3DES,
   eAlgAESCBC128,
   eAlgAESCBC192,
   eAlgAESCBC256,
   eAlgAESCTR128,
   eAlgAESCTR192,
   eAlgAESCTR256,
   eAlgAESCCM128,
   eAlgAESCCM192,
   eAlgAESCCM256,
   eAlgAESGCM128,
   eAlgAESGCM192,
   eAlgAESGCM256,
};

enum {
   eAuthNULL = 0,
   eAuthMD5,
   eAuthSHA1,
   eAuthSHA256,
   eAuthSHA384,
   eAuthSHA512,
   eAuthAESXCBC,
   eAuthAESCMAC,
   eAuthAESGMAC,
};

enum {
   TRANSPORT=1,
   TUNNEL
};

static void outdiff (unsigned char *buf1, unsigned char *buf2, int len)
{
  int x, y;
  char buf[16];
  printk("outdiff len == %d\n", len);
  for (x = 0; x < len; x += 4) {
//    if (memcmp(buf1, buf2, 4) == 0) { buf1 += 4; buf2 += 4; continue; }
    sprintf (buf, "%04x   ", x);
    printk (buf);
    for (y = 0; y < 4; y++) {
      sprintf (buf, "%02lx", (unsigned long) buf1[y] & 255UL);
      printk (buf);
    }
    printk (" - ");
    for (y = 0; y < 4; y++) {
      sprintf (buf, "%02lx", (unsigned long) buf2[y] & 255UL);
      printk (buf);
    }
    printk (" = ");
    for (y = 0; y < 4; y++) {
      sprintf (buf, "%02lx", (unsigned long) (buf1[y] ^ buf2[y]) & 255UL);
      printk (buf);
    }
    printk ("\n");
    buf1 += 4;
    buf2 += 4;
  }
}


/* get a line and skip blanks and comment lines */
static char glbuf[512];

static char *kfgets (char *dst, int size, struct file *file)
{
  mm_segment_t oldfs;
  int n, first;
  char *odst = dst;

  oldfs = get_fs ();
  set_fs (get_ds ());

  first = 1;
  while (size > 1) {
    n = file->f_op->read (file, dst, 1, &file->f_pos);
    if (n < 1 && first == 1) {
      set_fs (oldfs);
      return NULL;
    }
    first = 0;
    dst += n;
    --size;
    if (n < 1 || (dst[-n] == '\n' || dst[-n] == '\t'))
      break;
  }
  *dst = 0x00;
  set_fs (oldfs);
  return odst;
}

static char *get_line (struct file *in)
{
  char *p;
  unsigned long L;
  do {
    p = kfgets (glbuf, sizeof (glbuf) - 1, in);
  } while (p && sscanf (glbuf, "%lx", &L) != 1);
  return p;
}

static void read_words (struct file *infile, unsigned char *dst, int nwords, int little_endian)
{
  char *p;
  unsigned long L;
  unsigned long *pdst = (unsigned long *) dst;

  while (nwords-- && (p = get_line (infile))) {
    if (sscanf (p, "%08lx", &L) != 1) return;
    if (little_endian)
      *pdst++ = htonl(L);
    else
      *pdst++ = (L);
//    printk("Read %08lx\n", pdst[-1]);
  }
}

static int read_conf(struct file *infile, int *ip_version, int *no_headers, int *in_size, int *out_size, int *dst_op_mode)
{
   char *p = get_line(infile);
   if (p == NULL) return -1;
   if (sscanf(p, "%d, %d, %d, %d, %d", ip_version, no_headers, in_size, out_size, dst_op_mode) != 5) {
      return -1;
   }
   return 0;
}

static int read_crypto_conf(struct file *infile, int *direction, int *mode, int *transform, int *encryption_alg, int *authentication_alg, uint32_t *spi, int *ext_seq, int *aesmaclen)
{
   char *p = get_line(infile);
   if (p == NULL) return -1;
   if (sscanf(p, "%d, %d, %d, %d, %d, %zu, %d, %d", direction, mode, transform, encryption_alg, authentication_alg, spi, ext_seq, aesmaclen) != 8) {
      return -1;
   }
   return 0;
}

static char *authname(int authentication_alg)
{
   switch (authentication_alg) {
      case eAuthNULL:    return "MAC-NULL";
      case eAuthMD5:     return "HMAC-MD5";
      case eAuthSHA1:    return "HMAC-SHA1";
      case eAuthSHA256:  return "HMAC-SHA256";
      case eAuthSHA384:  return "HMAC-SHA384";
      case eAuthSHA512:  return "HMAC-SHA512";
      case eAuthAESXCBC: return "MAC-AES-XCBC";
      case eAuthAESCMAC: return "MAC-AES-CMAC";
      case eAuthAESGMAC: return "MAC-AES-GMAC";
      default: return 0;
   }
}

static char *encname(int encryption_alg)
{
   switch (encryption_alg) {
      case eAlgNULL:      return "NULL";
      case eAlgDES:       return "DES";
      case eAlg3DES:      return "3DES";
      case eAlgAESCBC128: return "AES-CBC-128";
      case eAlgAESCBC192: return "AES-CBC-192";
      case eAlgAESCBC256: return "AES-CBC-256";
      case eAlgAESCTR128: return "AES-CTR-128";
      case eAlgAESCTR192: return "AES-CTR-192";
      case eAlgAESCTR256: return "AES-CTR-256";
      case eAlgAESCCM128: return "AES-CCM-128";
      case eAlgAESCCM192: return "AES-CCM-192";
      case eAlgAESCCM256: return "AES-CCM-256";
      case eAlgAESGCM128: return "AES-GCM-128";
      case eAlgAESGCM192: return "AES-GCM-192";
      case eAlgAESGCM256: return "AES-GCM-256";
      default: return 0;
   }
}

static int authkeysize(int authentication_alg)
{
   switch (authentication_alg) {
      case eAuthMD5:    return 16;
      case eAuthSHA1:   return 20;
      case eAuthSHA256: return 32;
      case eAuthSHA384: return 48;
      case eAuthSHA512: return 64;
      case eAuthAESXCBC: return 64; // 3KEY mode
      case eAuthAESCMAC: return 16;
      case eAuthAESGMAC: return 16;
      default: return 0;
   }
}

static int enckeysize(int encryption_alg)
{
   switch (encryption_alg) {
      case eAlgNULL:      return 0;
      case eAlgDES:       return 8;
      case eAlg3DES:      return 24;
      case eAlgAESCBC128: return 16;
      case eAlgAESCBC192: return 24;
      case eAlgAESCBC256: return 32;
      case eAlgAESCTR128: return 16;
      case eAlgAESCTR192: return 24;
      case eAlgAESCTR256: return 32;
      case eAlgAESCCM128: return 16;
      case eAlgAESCCM192: return 24;
      case eAlgAESCCM256: return 32;
      case eAlgAESGCM128: return 16;
      case eAlgAESGCM192: return 24;
      case eAlgAESGCM256: return 32;
      default: return 0;
   }
}

static int encivsize(int encryption_alg)
{
   switch (encryption_alg) {
      case eAlgNULL:      return 0;
      case eAlgDES:       return 8;
      case eAlg3DES:      return 8;
      case eAlgAESCBC128: return 16;
      case eAlgAESCBC192: return 16;
      case eAlgAESCBC256: return 16;
      case eAlgAESCTR128: return 4;
      case eAlgAESCTR192: return 4;
      case eAlgAESCTR256: return 4;
      case eAlgAESCCM128: return 4;
      case eAlgAESCCM192: return 4;
      case eAlgAESCCM256: return 4;
      case eAlgAESGCM128: return 4;
      case eAlgAESGCM192: return 4;
      case eAlgAESGCM256: return 4;
      default: return 0;
   }
}

static uint32_t vectosdk_enc(int encryption_alg)
{
   switch (encryption_alg) {
      case eAlgNULL:      return EAPE_SA_ALG_CIPH_NULL;
      case eAlgDES:       return EAPE_SA_ALG_CIPH_DES_CBC;
      case eAlg3DES:      return EAPE_SA_ALG_CIPH_3DES_CBC;
      case eAlgAESCBC128: return EAPE_SA_ALG_CIPH_AES128_CBC;
      case eAlgAESCBC192: return EAPE_SA_ALG_CIPH_AES192_CBC;
      case eAlgAESCBC256: return EAPE_SA_ALG_CIPH_AES256_CBC;
      case eAlgAESCTR128: return EAPE_SA_ALG_CIPH_AES128_CTR;
      case eAlgAESCTR192: return EAPE_SA_ALG_CIPH_AES192_CTR;
      case eAlgAESCTR256: return EAPE_SA_ALG_CIPH_AES256_CTR;
      case eAlgAESGCM128: return EAPE_SA_ALG_CIPH_AES128_GCM;
      case eAlgAESGCM192: return EAPE_SA_ALG_CIPH_AES192_GCM;
      case eAlgAESGCM256: return EAPE_SA_ALG_CIPH_AES256_GCM;
//      case eAlgAESCCM128: return EAPE_SA_ALG_CIPH_AES128_CCM;
//      case eAlgAESCCM192: return EAPE_SA_ALG_CIPH_AES192_CCM;
//      case eAlgAESCCM256: return EAPE_SA_ALG_CIPH_AES256_CCM;
      default: return 0;
   }
}

static uint32_t vectosdk_auth(int authentication_alg)
{
   switch (authentication_alg) {
      case eAuthNULL:    return EAPE_SA_ALG_AUTH_NULL;
      case eAuthMD5:     return EAPE_SA_ALG_AUTH_MD5_96;
      case eAuthSHA1:    return EAPE_SA_ALG_AUTH_SHA1_96;
      case eAuthSHA256:  return EAPE_SA_ALG_AUTH_SHA256;
//      case eAuthSHA384:  return
//      case eAuthSHA512:  return
//      case eAuthAESXCBC: return
//      case eAuthAESCMAC: return
      case eAuthAESGMAC: return EAPE_SA_ALG_AUTH_GMAC;
      default: return 0;
   }
}

/* this function maps a virtual buffer to an SG then allocates a DDT and maps it, it breaks the buffer into 4K chunks */
static int map_buf_to_sg_ddt (const void *src, unsigned long srclen, struct scatterlist **sg, unsigned *sgents, pdu_ddt * ddt)
{
   unsigned x, n, y;
   struct scatterlist *sgtmp;

   *sgents = 0;
   *sg = NULL;

   n = (srclen + 4095) >> 12;
   *sg = kmalloc (n * sizeof (**sg), GFP_KERNEL);
   if (!*sg) {
      printk ("Out of memory allocating SG\n");
      return -1;
   }
   sg_init_table (*sg, n);
   for (x = 0; srclen; x++) {
      y = (srclen > 4096) ? 4096 : srclen;
      sg_set_buf (&(*sg)[x], src, y);
      srclen -= y;
      src += y;
   }
   x = DMA_MAP_SG (NULL, *sg, n, DMA_BIDIRECTIONAL);
   if (!x) {
      kfree (*sg);
      *sg = NULL;
   }
   pdu_ddt_init (ddt, x);
   for_each_sg (*sg, sgtmp, x, y) {
      pdu_ddt_add (ddt, (PDU_DMA_ADDR_T) sg_dma_address (sgtmp), sg_dma_len (sgtmp));
   }
   *sgents = x;
   return 0;
}

int noisy_mode_activated;

struct callbackdata {
  struct completion comp;
  uint32_t outlen,
           retcode,
           swid;
};


static void diag_callback(void *eape_dev, void *data, uint32_t payload_len, uint32_t retcode, uint32_t swid)
{
   struct callbackdata *cbdata = data;
   cbdata->retcode = retcode;
   if (retcode) {
      printk("Callback retcode is %zu...\n", retcode);
      cbdata->outlen = 0;
   } else {
      cbdata->outlen  = payload_len;
      cbdata->swid    = swid;
   }
   complete(&cbdata->comp);
}

static int parse_test_script(char *filename,int little_endian)
{
   unsigned char *v_in  = NULL,
                 *v_out = NULL,
                 *out   = NULL,
                  enckey[32],
                  enciv[16],
                  authkey[64], SA[256];

   uint32_t spi;

   int ip_version,
       no_headers,
       in_size,
       out_size,
       direction,
       transform,
       mode,
       encryption_alg,
       authentication_alg,
       dst_op_mode,
       ext_seq,
       aesmaclen;

   struct file   *infile;
   mm_segment_t   oldfs;

   int ret    = -1,
       handle = -1;

   unsigned sgents_src, sgents_dst;
   struct scatterlist *sgsrc, *sgdst;
   pdu_ddt src_ddt, dst_ddt;
   eape_device *eape;
   eape_sa      sa;

   struct callbackdata cbdata;

   sgents_src = 0;
   sgents_dst = 0;
   sgsrc = NULL;
   sgdst = NULL;
   v_out = NULL;
   v_in  = NULL;
     out = NULL;

   eape = eape_get_device();

   memset(enckey, 0, sizeof enckey);
   memset(enciv, 0, sizeof enciv);
   memset(authkey, 0, sizeof authkey);
   memset(SA, 0, sizeof SA);

// OPEN FILE
   oldfs = get_fs ();
   set_fs (get_ds ());
   infile = filp_open (filename, O_RDONLY, 0);
   if (IS_ERR (infile)) {
     set_fs (oldfs);
     printk("eapediag: cannot open vector %s\n", filename);
     return 2;
   }
   set_fs (oldfs);

// read in version, # of headers, input/output size
   if ((ret = read_conf(infile, &ip_version, &no_headers, &in_size, &out_size, &dst_op_mode)) != 0) {
      printk("eapediag: reading configuration failed\n");
      goto end;
   }
   if (no_headers > MAXHEADERS) {
      printk("eapediag: no_headers is too large\n");
      ret = -1;
      goto end;
   }

// read crypto conf
   if ((ret = read_crypto_conf(infile, &direction, &mode, &transform, &encryption_alg, &authentication_alg, &spi, &ext_seq, &aesmaclen)) != 0) {
      printk("eapediag: reading crypto conf failed\n");
      goto end;
   }

// read keys/iv
// we always read the AUTH key
//   printk("Reading authkey\n");
   if (authentication_alg == eAuthAESGMAC) {
      read_words(infile, enckey, 16/4,1);                               // GMAC key goes in cipher area ...
   } else {
      read_words(infile, authkey, authkeysize(authentication_alg)/4,1);
   }
   if (transform == ESP) {
//      printk("Reading enckey\n");
      read_words(infile, enckey, enckeysize(encryption_alg)/4,1);
//      printk("Reading enciv\n");
      if (authentication_alg == eAuthAESGMAC) {
         read_words(infile, enciv, 1,1);
      } else {
         read_words(infile, enciv, encivsize(encryption_alg)/4,1);
      }
   } else {
      if (authentication_alg == eAuthAESGMAC) {
         read_words(infile, enciv, 1,1);
      }
   }


// read input
   v_in = kmalloc(in_size, GFP_KERNEL);
   if (!v_in) {
     printk("eapediag: error allocating memory for v_in\n");
     goto end;
   }

   v_out = kzalloc(out_size, GFP_KERNEL);
   if (!v_out) {
     printk("eapediag: error allocating memory for v_out\n");
     goto end;
   }

   out = kmalloc(out_size, GFP_KERNEL);
   if (!out) {
     printk("eapediag: error allocating memory for out\n");
     goto end;
   }

//   printk("READING INPUT\n");
   read_words(infile, v_in,  (in_size+3)/4,little_endian);
//   printk("READING OUTPUT\n");
   read_words(infile, v_out, (out_size+3)/4,little_endian);

   map_buf_to_sg_ddt(v_in, in_size,  &sgsrc, &sgents_src, &src_ddt);
   map_buf_to_sg_ddt(out,  out_size, &sgdst, &sgents_dst, &dst_ddt);

   DMA_SYNC_SG_FOR_DEVICE(NULL, sgsrc, sgents_src, DMA_BIDIRECTIONAL);

   // run vector
   // allocate handle
   handle = eape_open(eape);
   if (handle < 0) {
      printk("eapediag:  Failed to open an ESPAH handle\n");
      ret = -1;
      goto end;
   }

   // buildSA
   memset(&sa, 0, sizeof sa);
   sa.alg   = vectosdk_enc(encryption_alg)        |
              vectosdk_auth(authentication_alg);
   sa.flags = EAPE_SA_FLAGS_ACTIVE                             |
              ((ip_version == 6) ? EAPE_SA_FLAGS_IPV6 : 0)     |
              ((transform == AH) ? EAPE_SA_FLAGS_HDR_TYPE : 0) |
              (dst_op_mode ? EAPE_SA_FLAGS_DST_OP_MODE : 0);
   sa.spi   = spi;
   memcpy(sa.ckey, enckey, 32);
   memcpy(sa.csalt, enciv, 4);
   memcpy(sa.mackey, authkey, 64);
   ret = eape_build_sa(eape, handle, &sa);
   if (ret< 0) {
      printk("eapediag::Error building sa %d\n", ret);
      goto end;
   }

   // do the job
   init_completion(&cbdata.comp);
   ret = eape_go(eape, handle, (direction == OUTBOUND) ? 1 : 0, &src_ddt, &dst_ddt, diag_callback, &cbdata);
   if (wait_for_completion_interruptible(&cbdata.comp)) {
      ret = 1;
      printk("eapediag::User aborted vector\n");
      msleep(1000); // sleep here because if the engine isn't locked up it might write to our DMA'ables after we free ... no single job should take >1sec
      goto end;
   }

   DMA_SYNC_SG_FOR_CPU(NULL, sgdst, sgents_dst, DMA_BIDIRECTIONAL);

// compare
   if (cbdata.retcode || memcmp((uint32_t *)out, (uint32_t *)v_out, cbdata.outlen)) {
      outdiff(out, v_out, cbdata.outlen);
      printk("eapediag: Output differs from what was expected\n");
      ret = -1;
      goto end;
   }

// at this point we're all good
   ret = 0;
end:
   if (ret || noisy_mode_activated) {
      printk("eapediag: vector [IP%d, %s, %s, in_size == %d, %s, %s, %s] == [%s]\n", ip_version, encname(encryption_alg), authname(authentication_alg), in_size,
             transform == AH ? "AH" : "ESP", mode == TUNNEL ? "TUNNEL" : "TRANSPORT", direction == OUTBOUND ? "OUTBOUND" : "INBOUND", (ret == 0) ? "PASSED" : "FAILED");
   }

   // have to unmap before free
   if (sgents_src) {
      DMA_UNMAP_SG (NULL, sgsrc, sgents_src, DMA_BIDIRECTIONAL);
      kfree (sgsrc);
      pdu_ddt_free (&src_ddt);
   }

   if (sgents_dst) {
      DMA_UNMAP_SG (NULL, sgdst, sgents_dst, DMA_BIDIRECTIONAL);
      kfree (sgdst);
      pdu_ddt_free (&dst_ddt);
   }

   if (v_in) {
      kfree(v_in);
   }
   if (out) {
      kfree(out);
   }
   if (v_out) {
      kfree(v_out);
   }

   oldfs = get_fs ();
   set_fs (get_ds ());
   filp_close (infile, NULL);
   set_fs (oldfs);

   eape_close(eape, handle);

   return ret;
}

static int test_eape_file (struct file *file, const char *buffer, unsigned long count, void *data)
{
  int err;
  char test_vector_name[256];

  memset (test_vector_name, 0, sizeof test_vector_name);
  if (count > sizeof test_vector_name)
    return -EINVAL;
  if (copy_from_user (test_vector_name, buffer, count))
    return -EFAULT;

  // remove new line
  test_vector_name[strlen (test_vector_name) - 1] = '\0';
  err = parse_test_script (test_vector_name, 1);

  return (err == 0) ? count : -EINVAL;
}


static int __init eapediag_mod_init (void)
{
   // register proc
   struct proc_dir_entry *proc_ea;

   proc_ea = create_proc_entry ("eapediag", 0, 0);
   if (!proc_ea) {
      printk ("re_proc_init: cannot create /proc/eapediag\n");
      return -ENOMEM;
   }
   proc_ea->write_proc = test_eape_file;
   return 0;
}

static void __exit eapediag_mod_exit (void)
{
   // unregister proc
   remove_proc_entry ("eapediag", 0);
}

MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("Elliptic Technologies Inc.");
module_init (eapediag_mod_init);
module_exit (eapediag_mod_exit);
module_param(noisy_mode_activated, int, 0);
MODULE_PARM_DESC(noisy_mode_activated, "verbose diag");

