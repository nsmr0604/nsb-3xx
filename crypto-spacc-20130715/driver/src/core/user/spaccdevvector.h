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
//   spacc vector parser header
//
//-----------------------------------------------------------------------*/
#ifndef _ELPPARSER_H
#define _ELPPARSER_H

enum
{
   FAILURE_FLAG = 0,
   MULTI2_FLAG  = 1,
   AUXINFO_FLAG = 2,
};

enum{
   ERROR_KEY         = 0,
   ERROR_IV          = 1,
   ERROR_SALT_KEY    = 2,
   ERROR_ICV_KEY     = 3,
   ERROR_INPUT       = 4,
};

#define SEK_KEY_MASK 0x00008000
#define SEK_KEY_SHIFT 15
#define MSEK_KEY_MASK 0x00004000
#define MSEK_KEY_SHIFT 14

/* enc_mode index */
enum
{
  VEC_MODE_NULL = 0,
  VEC_MODE_RC4_40,
  VEC_MODE_RC4_128,
  VEC_MODE_RC4_KS,
  VEC_MODE_AES_ECB,
  VEC_MODE_AES_CBC,
  VEC_MODE_AES_CTR,
  VEC_MODE_AES_CCM,
  VEC_MODE_AES_GCM,
  VEC_MODE_AES_F8,
  VEC_MODE_AES_XTS,
  VEC_MODE_AES_CFB,
  VEC_MODE_AES_OFB,
  VEC_MODE_AES_CS1,
  VEC_MODE_AES_CS2,
  VEC_MODE_AES_CS3,
  VEC_MODE_MULTI2_ECB,
  VEC_MODE_MULTI2_CBC,
  VEC_MODE_MULTI2_OFB,
  VEC_MODE_MULTI2_CFB,
  VEC_MODE_3DES_CBC,
  VEC_MODE_3DES_ECB,
  VEC_MODE_DES_CBC,
  VEC_MODE_DES_ECB,
  VEC_MODE_KASUMI_ECB,
  VEC_MODE_KASUMI_F8,
  VEC_MODE_SNOW3G_UEA2,
  VEC_MODE_ZUC_UEA3,
};

/* hash mode index */
enum
{
   VEC_MODE_HASH_NULL = 0,
   VEC_MODE_HASH_SHA1,
   VEC_MODE_HMAC_SHA1,
   VEC_MODE_HASH_MD5,
   VEC_MODE_HMAC_MD5,
   VEC_MODE_HASH_SHA224,
   VEC_MODE_HMAC_SHA224,
   VEC_MODE_HASH_SHA256,
   VEC_MODE_HMAC_SHA256,
   VEC_MODE_HASH_SHA384,
   VEC_MODE_HMAC_SHA384,
   VEC_MODE_HASH_SHA512,
   VEC_MODE_HMAC_SHA512,
   VEC_MODE_HASH_SHA512_224,
   VEC_MODE_HMAC_SHA512_224,
   VEC_MODE_HASH_SHA512_256,
   VEC_MODE_HMAC_SHA512_256,
   VEC_MODE_MAC_XCBC,
   VEC_MODE_MAC_CMAC,
   VEC_MODE_MAC_KASUMI_F9,
   VEC_MODE_SSLMAC_MD5,
   VEC_MODE_SSLMAC_SHA1,
   VEC_MODE_MAC_SNOW3G_UIA2,
   VEC_MODE_MAC_ZUC_UIA3,
   VEC_MODE_HASH_CRC32,
};



// Generic Test Vector structure collects all control and data fiels from the
// binary vector format

typedef struct vector_data_
{
   int enc_mode;
   int hash_mode;
   int keysize;
   int hmac_keysize;

   int iv_offset;
   int iv_size;
   int c_iv_size;
   int h_iv_size;

   int icv_mode;
   int icv_offset;
   int icv_size;
   int icvpos;    // this is set interally it's the flag of how the ICV offset is loaded
   unsigned char icv[128];

   int salt_size;
   int flags;
   int errors;
   int fail_mode;
   int outsplit[10];
   int insplit[10];

   int pre_aad_size;
   int post_aad_size;
   int pt_size;
   int ct_size;

   int auxinfo_dir;
   int auxinfo_bit_align;

   int seckey;
   int mseckey;

   unsigned long seed;

   unsigned char key[384];
   unsigned char hmac_key[384];
   unsigned char iv[384];
   unsigned char saltkey[384];

   unsigned char *pre_aad;
   unsigned char *post_aad;
   unsigned char *pt;
   unsigned char *ct;

   unsigned int epn, virt;

   struct {
       unsigned char *pool, *pread, *postad, *ct, *pt;
   } mem;

   size_t max_packet;

   int error_field, error_mangle, error_offset;
} vector_data;

// This function reads binary test vector content and stores it in the
// vector_data structure. Note that it allocates the input and output data
// pointers so do not for get to call the free_test_vector function for each
// vector parsed
int parse_test_vector (char *filename, vector_data *vector);
vector_data *init_test_vector(size_t max_packet);
void deinit_test_vector(void);
int free_test_vector(vector_data *vector);

#endif
