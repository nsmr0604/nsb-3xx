/*-----------------------------------------------------------------------
//
// Proprietary Information of Elliptic Technologies
// Copyright (C) 2002-2012, all rights reserved
// Elliptic Technologies Inc.
//
// As part of our confidentiality  agreement, Elliptic  Technologies and
// the Company, as  a  Receiving Party, of  this  information  agrees to
// keep strictly  confidential  all Proprietary Information  so received
// from Elliptic  Technologies. Such Proprietary Information can be used
// solely for  the  purpose  of evaluating  and/or conducting a proposed
// business  relationship  or  transaction  between  the  parties.  Each
// Party  agrees  that  any  and  all  Proprietary  Information  is  and
// shall remain confidential and the property of Elliptic  Technologies.
// The  Company  may  not  use  any of  the  Proprietary  Information of
// Elliptic  Technologies for any purpose other  than  the  above-stated
// purpose  without the prior written consent of Elliptic  Technologies.
//
//-----------------------------------------------------------------------
//
// Project:
//
//   common sdk
//
// Description:
//
//   parses spacc vectors
//
//-----------------------------------------------------------------------*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/io.h>

#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include "elpparser.h"

typedef struct file elp_file;

// OPEN FILE
static elp_file   *elp_fopen(char *filename)
{
   elp_file   *in;
  mm_segment_t   oldfs;

  oldfs = get_fs ();
  set_fs (get_ds ());
  in = filp_open (filename, O_RDONLY, 0);
  if (IS_ERR (in)) {
    set_fs (oldfs);
    return NULL;
  }
  set_fs (oldfs);

   return in;
}

static void elp_fclose( elp_file   *in)
{
  mm_segment_t   oldfs;
  oldfs = get_fs ();
  set_fs (get_ds ());
  filp_close (in, NULL);
  set_fs (oldfs);
}

static int elp_fgetc (elp_file * file){
  mm_segment_t oldfs;
  char c[1];

  oldfs = get_fs();
  set_fs (get_ds());

  file->f_op->read(file, c , 1, &file->f_pos);
  set_fs (oldfs);
  return c[0] & 0xFF;

}

static size_t elp_fread (elp_file * file, unsigned char *dst, size_t len){
  mm_segment_t oldfs;
  size_t n;

  oldfs = get_fs();
  set_fs (get_ds());

  n = file->f_op->read(file, dst, len, &file->f_pos);
  set_fs (oldfs);
  return n;
}

#define PARSE_MALLOC(x) vmalloc(x)
#define PARSE_FREE(x) vfree(x)

void printHex(char * label, char * digits, unsigned long length){
    int x;
    unsigned long linenumber = 0;
    if (length == 0) return;
    printk("#%s %lu: \n %06lx: ", label, length, linenumber);
    for (x = 0; x < length;){
       printk("%02lx", (unsigned long)(digits[x] & 0xFF));
       if ((x & 0x01) == 1){
          printk(" ");
       }
       if (((++x & 0x07) == 0) && x != length){
          printk("\n %06lx: ", linenumber += 0x08);
       }
    }
    printk("\n");
}


int getNum(int length, elp_file * file){
  unsigned int temp, i;
  unsigned char c;
  temp = 0;
  if (length == 0) return 0;
  for (i = 0; i < length; i+=1){
     c = elp_fgetc(file);
     temp |= (c & 0xFF) << 8*(length - 1 - i);
  }
  return temp;
}

void getChars(unsigned char * buffer, int length, elp_file * file){
    if (elp_fread(file, buffer, length) != length) {
       printk("SPACC VECTOR: Warning did not read entire buffer into memory\n");
    }
}

int getSplits(int * splits, elp_file * file){
    int numofsplits = 0;
    int i = 0;

    numofsplits = getNum(1, file);
    for (i = 0; i < numofsplits; i += 1){
       splits[i] = getNum(4, file);
       if (splits[i] == -1){
          return -1;
       }
    }
    return 1;
}

vector_data *init_test_vector(size_t max_packet)
{
   vector_data *vector;

   vector = PARSE_MALLOC(sizeof *vector);
   if (vector == NULL) return NULL;

   memset(vector,0,sizeof(vector_data));
   vector->max_packet = max_packet;

   vector->mem.pool = PARSE_MALLOC(4*max_packet);
   if (vector->mem.pool == NULL) { PARSE_FREE(vector); return NULL; }
   vector->mem.pread  = vector->mem.pool + (0 * max_packet);
   vector->mem.postad = vector->mem.pool + (1 * max_packet);
   vector->mem.pt     = vector->mem.pool + (2 * max_packet);
   vector->mem.ct     = vector->mem.pool + (3 * max_packet);
   return vector;
}

void deinit_test_vector(void)
{
}


int free_test_vector(vector_data *vector)
{
   PARSE_FREE(vector->mem.pool);
   PARSE_FREE(vector);
   return 1;
}

// This function reads binary test vector content and stores it in the
// vector_data structure. Note that it allocates the input and output data
// pointers so remember to call the free_test_vector function for each
// vector parsed
int parse_test_vector (char *filename, vector_data *vector)
{
  int y=-1;
  elp_file   *in;

   if ((in = elp_fopen(filename)) == NULL)
      return -1;

  vector->seed      = getNum(4, in);
  vector->flags     = getNum(1, in);
  vector->errors    = getNum(1, in);
  vector->error_field  = getNum(1, in);
  vector->error_mangle = getNum(1, in);
  vector->error_offset = getNum(4, in);
  vector->enc_mode  = getNum(1, in);
  vector->hash_mode = getNum(1, in);
  vector->icv_mode  = getNum(1, in);
  vector->iv_offset = getNum(4, in);

  if (vector->iv_offset == 0xFFFFFFFF) {
     vector->iv_offset = 0;
  } else {
     vector->iv_offset |= 0x80000000UL;   // if we are using it we must enable the high bit
  }

  if ((vector->flags >> AUXINFO_FLAG) & 0x01){
    vector->auxinfo_bit_align = getNum(1, in);
    vector->auxinfo_dir = getNum(1, in);
  } else {
    vector->auxinfo_bit_align = 8;
    vector->auxinfo_dir = 0;
  }

  if ((vector->flags >> FAILURE_FLAG) & 0x01){
     vector->fail_mode = 1;
  } else {
     vector->fail_mode = 0;
  }
  vector->keysize = getNum(2, in);
  vector->seckey  = (vector->keysize & SEK_KEY_MASK) >> SEK_KEY_SHIFT;
  vector->mseckey  = (vector->keysize & MSEK_KEY_MASK) >> MSEK_KEY_SHIFT;
  vector->keysize &= ~(SEK_KEY_MASK | MSEK_KEY_MASK);
  if (vector->keysize > sizeof(vector->key)) {
     printk("SPACC VECTOR:  Keysize larger than max key size supported\n");
     goto ERR;
  }
  getChars(vector->key, vector->keysize, in);

  vector->hmac_keysize = getNum(1, in);
  if (vector->hmac_keysize > sizeof(vector->hmac_key)) {
     printk("SPACC VECTOR:  HASH Keysize larger than max key size supported\n");
     goto ERR;
  }
  getChars(vector->hmac_key, vector->hmac_keysize, in);

  vector->iv_size = getNum(2, in);

  vector->c_iv_size = vector->iv_size & 0xff;
  vector->h_iv_size = (vector->iv_size & 0xff00) >> 8;
  vector->iv_size = vector->c_iv_size + vector->h_iv_size;
   if (vector->iv_offset != 0) {
      vector->c_iv_size = 0;
      vector->h_iv_size = 0;
   }

  if (vector->iv_size > sizeof(vector->iv)) {
     printk("SPACC VECTOR:  IVsize larger than max IV size supported\n");
     goto ERR;
  }
  getChars(vector->iv, vector->iv_size, in);

  vector->salt_size = getNum(1, in);
  if (vector->salt_size > sizeof(vector->saltkey)) {
     printk("SPACC VECTOR:  salt_size larger than max salt size supported\n");
     goto ERR;
  }
  getChars(vector->saltkey, vector->salt_size, in);

  vector->pre_aad_size = getNum(4, in);
  if (vector->pre_aad_size > vector->max_packet) {
     printk("SPACC VECTOR:  PRE AAD size too large\n");
     goto ERR;
  }

  if (vector->pre_aad_size) {
     vector->pre_aad = vector->mem.pread;
     getChars(vector->pre_aad, vector->pre_aad_size, in);
  }

  vector->post_aad_size = getNum(4, in);
  if (vector->post_aad_size > vector->max_packet) {
     printk("SPACC VECTOR:  POST AAD size too large\n");
     goto ERR;
  }
  if (vector->post_aad_size) {
     vector->post_aad = vector->mem.postad;
     getChars(vector->post_aad, vector->post_aad_size, in);
  }

  vector->pt_size = getNum(4, in);
  if (vector->pt_size > vector->max_packet) {
     printk("SPACC VECTOR:  PT size too large\n");
     goto ERR;
  }

  vector->pt = vector->mem.pt;
  getChars(vector->pt, vector->pt_size, in);

  vector->ct_size = getNum(4, in);
  if (vector->ct_size > vector->max_packet) {
     printk("SPACC VECTOR:  CT size too large\n");
     goto ERR;
  }
  vector->ct = vector->mem.ct;
  getChars(vector->ct, vector->ct_size, in);

  vector->icv_size = getNum(1, in);
  if (vector->icv_size > sizeof(vector->icv)) {
     printk("SPACC VECTOR:  ICV size too large\n");
     goto ERR;
  }
  getChars(vector->icv, vector->icv_size, in);
  // for now, the ICV offset < SPACC_MAX_PARTICLE_SIZE (65536)
  vector->icv_offset = getNum(4, in);

  vector->virt = getNum(4,in);
  vector->epn = getNum(4,in);

  y = 0;
ERR:
  elp_fclose(in);
  return y;
}



