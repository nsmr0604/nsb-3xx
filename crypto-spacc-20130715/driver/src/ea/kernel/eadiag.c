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
#include <linux/device.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <asm/param.h>
#include "elpea.h"
#include "elpeahw.h"
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
  return; // by default we don't want long outputs
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
//    printk("Read %08lx\n", L);
    if (little_endian)
      *pdst++ = htonl(L);
    else
      *pdst++ = (L);
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

static uint32_t vectosdk_enc(int encryption_alg, uint32_t aesmaclen)
{
   switch (aesmaclen) {
       case  8:  aesmaclen = MAC_LEN_64; break;
       case 12:  aesmaclen = MAC_LEN_96; break;
       case 16:  aesmaclen = MAC_LEN_128; break;
   }
   switch (encryption_alg) {
      case eAlgNULL:      return SA_CTRL_CIPH_ALG(CIPH_ALG_NULL);
      case eAlgDES:       return SA_CTRL_CIPH_ALG(CIPH_ALG_DES_CBC)|SA_CTRL_CKEY_LEN(CKEY_LEN_128);
      case eAlg3DES:      return SA_CTRL_CIPH_ALG(CIPH_ALG_DES_CBC)|SA_CTRL_CKEY_LEN(CKEY_LEN_192);
      case eAlgAESCBC128: return SA_CTRL_CIPH_ALG(CIPH_ALG_AES_CBC)|SA_CTRL_CKEY_LEN(CKEY_LEN_128);
      case eAlgAESCBC192: return SA_CTRL_CIPH_ALG(CIPH_ALG_AES_CBC)|SA_CTRL_CKEY_LEN(CKEY_LEN_192);
      case eAlgAESCBC256: return SA_CTRL_CIPH_ALG(CIPH_ALG_AES_CBC)|SA_CTRL_CKEY_LEN(CKEY_LEN_256);
      case eAlgAESCTR128: return SA_CTRL_CIPH_ALG(CIPH_ALG_AES_CTR)|SA_CTRL_CKEY_LEN(CKEY_LEN_128);
      case eAlgAESCTR192: return SA_CTRL_CIPH_ALG(CIPH_ALG_AES_CTR)|SA_CTRL_CKEY_LEN(CKEY_LEN_192);
      case eAlgAESCTR256: return SA_CTRL_CIPH_ALG(CIPH_ALG_AES_CTR)|SA_CTRL_CKEY_LEN(CKEY_LEN_256);
      case eAlgAESCCM128: return SA_CTRL_CIPH_ALG(CIPH_ALG_AES_CCM)|SA_CTRL_CKEY_LEN(CKEY_LEN_128)|SA_CTRL_MAC_LEN(aesmaclen);
      case eAlgAESCCM192: return SA_CTRL_CIPH_ALG(CIPH_ALG_AES_CCM)|SA_CTRL_CKEY_LEN(CKEY_LEN_192)|SA_CTRL_MAC_LEN(aesmaclen);
      case eAlgAESCCM256: return SA_CTRL_CIPH_ALG(CIPH_ALG_AES_CCM)|SA_CTRL_CKEY_LEN(CKEY_LEN_256)|SA_CTRL_MAC_LEN(aesmaclen);
      case eAlgAESGCM128: return SA_CTRL_CIPH_ALG(CIPH_ALG_AES_GCM)|SA_CTRL_CKEY_LEN(CKEY_LEN_128)|SA_CTRL_MAC_LEN(aesmaclen);
      case eAlgAESGCM192: return SA_CTRL_CIPH_ALG(CIPH_ALG_AES_GCM)|SA_CTRL_CKEY_LEN(CKEY_LEN_192)|SA_CTRL_MAC_LEN(aesmaclen);
      case eAlgAESGCM256: return SA_CTRL_CIPH_ALG(CIPH_ALG_AES_GCM)|SA_CTRL_CKEY_LEN(CKEY_LEN_256)|SA_CTRL_MAC_LEN(aesmaclen);
      default: return 0;
   }
}

static uint32_t vectosdk_auth(int authentication_alg)
{
   switch (authentication_alg) {
      case eAuthNULL:    return SA_CTRL_MAC_ALG(MAC_ALG_NULL);
      case eAuthMD5:     return SA_CTRL_MAC_ALG(MAC_ALG_HMAC_MD5_96);
      case eAuthSHA1:    return SA_CTRL_MAC_ALG(MAC_ALG_HMAC_SHA1_96);
      case eAuthSHA256:  return SA_CTRL_MAC_ALG(MAC_ALG_HMAC_SHA256_128);
      case eAuthSHA384:  return SA_CTRL_MAC_ALG(MAC_ALG_HMAC_SHA384_192);
      case eAuthSHA512:  return SA_CTRL_MAC_ALG(MAC_ALG_HMAC_SHA512_256);
      case eAuthAESXCBC: return SA_CTRL_MAC_ALG(MAC_ALG_AES_XCBC_MAC_96);
      case eAuthAESCMAC: return SA_CTRL_MAC_ALG(MAC_ALG_AES_CMAC_96);
      case eAuthAESGMAC: return SA_CTRL_CIPH_ALG(CIPH_ALG_AES_GMAC)|SA_CTRL_MAC_LEN(MAC_LEN_128);
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
      return -1;
   }
   if (pdu_ddt_init (ddt, x) < 0) {
      DMA_UNMAP_SG (NULL, *sg, x, DMA_BIDIRECTIONAL);
      kfree(*sg);
      return -1;
   }
   for_each_sg (*sg, sgtmp, x, y) {
      pdu_ddt_add (ddt, (PDU_DMA_ADDR_T) sg_dma_address (sgtmp), sg_dma_len (sgtmp));
   }
   *sgents = x;
   return 0;
}

int noisy_mode_activated;

struct callbackdata {
  struct completion comp;
  volatile uint32_t outlen,
           retcode,
           swid,
           sttl;
};


static void diag_callback(void *ea_dev, void *data, uint32_t payload_len, uint32_t retcode, uint32_t sttl, uint32_t swid)
{
   struct callbackdata *cbdata = data;
   cbdata->retcode = retcode;
   if (retcode) {
      printk("Callback retcode is %zu...\n", retcode);
      cbdata->outlen = 0;
   } else {
      cbdata->outlen  = payload_len;
      cbdata->sttl    = sttl;
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
       headers[MAXHEADERS],
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
   ea_device *ea;
   ea_sa      sa;

   PDU_DMA_ADDR_T v_in_phys, v_out_phys;

   volatile struct callbackdata cbdata;

   sgents_src = 0;
   sgents_dst = 0;
   sgsrc = NULL;
   sgdst = NULL;
   v_out = NULL;
   v_in  = NULL;
     out = NULL;

   ea = ea_get_device();

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
     printk("elpespah: cannot open vector %s\n", filename);
     return 2;
   }
   set_fs (oldfs);

// read in version, # of headers, input/output size
   if ((ret = read_conf(infile, &ip_version, &no_headers, &in_size, &out_size, &dst_op_mode)) != 0) {
      printk("elpespah: reading configuration failed\n");
      goto end;
   }
   if (no_headers > MAXHEADERS) {
      printk("elpespah: no_headers is too large\n");
      ret = -1;
      goto end;
   }

// read crypto conf
   if ((ret = read_crypto_conf(infile, &direction, &mode, &transform, &encryption_alg, &authentication_alg, &spi, &ext_seq, &aesmaclen)) != 0) {
      printk("elpespah: reading crypto conf failed\n");
      goto end;
   }

#if 0
   printk("ip_version = %d\nno_headers = %d\nin_size    = %d\nout_size   = %d\n", ip_version, no_headers, in_size, out_size);
   printk("direction  = %d\nmode       = %d\ntransform  = %d\nencalg     = %d\nauthalg    = %d\nSPI = %d\n", direction, mode, transform, encryption_alg, authentication_alg, spi);
#endif


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
     printk("elpespah: error allocating memory for v_in\n");
     goto end;
   }

   v_out = kzalloc(out_size, GFP_KERNEL);
   if (!v_out) {
     printk("elpespah: error allocating memory for v_out\n");
     goto end;
   }

   out = kmalloc(out_size, GFP_KERNEL);
   if (!out) {
     printk("elpespah: error allocating memory for out\n");
     goto end;
   }

//   printk("READING INPUT\n");
   read_words(infile, v_in,  (in_size+3)/4,little_endian);
//   printk("READING OUTPUT\n");
   read_words(infile, v_out, (out_size+3)/4,little_endian);

   if (map_buf_to_sg_ddt(v_in, in_size,  &sgsrc, &sgents_src, &src_ddt) < 0) {
     goto end_no_dma;
   }

   if (map_buf_to_sg_ddt(out,  out_size, &sgdst, &sgents_dst, &dst_ddt) < 0) {
     goto end_src_dma;
   }

   DMA_SYNC_SG_FOR_DEVICE(NULL, sgsrc, sgents_src, DMA_BIDIRECTIONAL);

   // run vector
   // allocate handle
   handle = ea_open(ea, 0);
   if (handle < 0) {
      printk("elpespah:  Failed to open an ESPAH handle\n");
      ret = -1;
      goto end;
   }

   // buildSA
   memset(&sa, 0, sizeof sa);
   sa.ctrl = vectosdk_enc(encryption_alg, aesmaclen)    |
             vectosdk_auth(authentication_alg)          |
             SA_CTRL_ACTIVE                             |
             (ext_seq ? SA_CTRL_ESN : 0)                |
             ((ip_version == 6) ? SA_CTRL_IPV6 : 0)     |
             ((transform == AH) ? SA_CTRL_HDR_TYPE : 0) |
             (dst_op_mode ? SA_CTRL_DST_OP_MODE : 0);
   sa.spi  = spi;
   memcpy(sa.ckey, enckey, 32);
   memcpy(sa.csalt, enciv, 4);
   memcpy(sa.mackey, authkey, 64);
   ret= ea_build_sa(ea, handle, &sa);
   if (ret< 0) {
      printk("eadiag::Error building sa %d\n", ret);
      goto end;
   }

   // do the job
   init_completion(&cbdata.comp);
   ret = ea_go(ea, handle, (direction == OUTBOUND) ? 1 : 0, &src_ddt, &dst_ddt, diag_callback, &cbdata);
   if (wait_for_completion_interruptible(&cbdata.comp)) {
      ret = 1;
      printk("eadiag::User aborted vector\n");
      printk("eadiag::%08zx\n", pdu_io_read32(ea->regmap + EA_IRQ_STAT));
      msleep(1000); // sleep here because if the engine isn't locked up it might write to our DMA'ables after we free ... no single job should take >1sec
      goto end;
   }

   DMA_SYNC_SG_FOR_CPU(NULL, sgdst, sgents_dst, DMA_BIDIRECTIONAL);

// compare
   if (cbdata.retcode || memcmp((uint32_t *)out, (uint32_t *)v_out, cbdata.outlen)) {
      outdiff(out, v_out, cbdata.outlen);
      printk("elpespah: Output differs from what was expected\n");
      ret = -1;
      goto end;
   }

// at this point we're all good
   ret = 0;
end:
   // have to unmap before free
   if (sgents_dst) {
      DMA_UNMAP_SG (NULL, sgdst, sgents_dst, DMA_BIDIRECTIONAL);
      kfree (sgdst);
      pdu_ddt_free (&dst_ddt);
   }
end_src_dma:
   if (sgents_src) {
      DMA_UNMAP_SG (NULL, sgsrc, sgents_src, DMA_BIDIRECTIONAL);
      kfree (sgsrc);
      pdu_ddt_free (&src_ddt);
   }
end_no_dma:
   if (ret || noisy_mode_activated) {
      printk("elpespah: vector [IP%d, %s, %s, in_size == %d, %s, %s, %s] == [%s]\n", ip_version, encname(encryption_alg), authname(authentication_alg), in_size,
             transform == AH ? "AH" : "ESP", mode == TUNNEL ? "TUNNEL" : "TRANSPORT", direction == OUTBOUND ? "OUTBOUND" : "INBOUND", (ret == 0) ? "PASSED" : "FAILED");
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

   ea_close(ea, handle);

   //if (handle != -1) {
//   espah_close(handle);
   //}


   return ret;
}

static ssize_t
store_eadiag(struct class *class, struct class_attribute *classattr,
                                  const char *buf, size_t count)
{
   char *c, test_vector_name[count+1];
   int err;

   strcpy(test_vector_name, buf);

   /*
    * Check for trailing newline and remove it.  We do not
    * support filenames containing newlines.
    */
   c = strchr(test_vector_name, '\n');
   if (c) {
      if (c[1] != '\0')
         return -EINVAL;
      *c = '\0';
   }

   err = parse_test_script (test_vector_name,1);
   return (err == 0) ? count : -EINVAL;
}

static struct class_attribute attrs[] = {
   __ATTR(vector, 0200, NULL, store_eadiag),
   __ATTR_NULL
};

static struct class eadiag_class = {
   .class_attrs = attrs,
   .owner = THIS_MODULE,
   .name = "eadiag",
};

static int __init eadiag_mod_init (void)
{
   return class_register(&eadiag_class);
}

static void __exit eadiag_mod_exit (void)
{
   class_unregister(&eadiag_class);
}

MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("Elliptic Technologies Inc.");
module_init (eadiag_mod_init);
module_exit (eadiag_mod_exit);
module_param(noisy_mode_activated, int, 0);
MODULE_PARM_DESC(noisy_mode_activated, "verbose diag");

