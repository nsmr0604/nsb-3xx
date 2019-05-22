/*
 * Copyright (c) 2013 Elliptic Technologies Inc.
 */

#include "elpkepuser.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static struct {
   int op, option, outlen, inlen, labellen, master_len, server_len, client_len;
   unsigned char label[32],
                 seed[48],
                 master_secret[48],
                 server_secret[48],
                 client_secret[48],
                 input[512],
                 output[512];
} *vec;

static int parse_vector(void)
{
   int x, n;
   unsigned char vector[768];

   read(0, vector, 768);
   memset(vec, 0, sizeof vec);
   x = 0;
   vec->op      = vector[x++];
   vec->option  = vector[x++];
   if (vector[x] > (sizeof(vec->label) - 1)) {
      printf("parse_vector::Invalid label length\n");
      return -1;
   }
   vec->labellen = vector[x];
   memcpy(vec->label, vector+x+1, vector[x]);
   x += vector[x]; ++x;
   vec->master_len = vector[x++]; if (vec->master_len > 48) { goto size_error; } memcpy(vec->master_secret, vector+x, vec->master_len); x += vec->master_len;
   vec->server_len = vector[x++]; if (vec->server_len > 48) { goto size_error; } memcpy(vec->server_secret, vector+x, vec->server_len); x += vec->server_len;
   vec->client_len = vector[x++]; if (vec->client_len > 48) { goto size_error; } memcpy(vec->client_secret, vector+x, vec->client_len); x += vec->client_len;
   n  = (vector[x++] << 8);
   n |= vector[x++];
   vec->outlen = n;
   if (n > sizeof vec->output) {
      printf("parse_vector::Invalid output length\n");
      return -1;
   }
   memcpy(vec->output, vector+x, n);
   return 0;
size_error:
   printf("Key component too large\n");
   return -1;
}

int main(void)
{
  int x;
  unsigned char out[512];

  vec = malloc(sizeof *vec);
  if (parse_vector() < 0) {
     return -1;
  }

  switch (vec->op) {
      case KEP_SSL3_KEYGEN:
         x = kep_ssl3_keygen(vec->master_secret, vec->server_secret, vec->client_secret, out, vec->outlen);
         break;
      case KEP_SSL3_SIGN:
         x = kep_ssl3_sign(vec->server_secret, vec->server_len, vec->option, vec->master_secret, out, vec->outlen);
         break;
      case KEP_TLS_PRF:
         x = kep_tls_prf(vec->label, vec->labellen, vec->option, vec->master_secret, vec->server_secret, vec->server_len, out, vec->outlen);
         break;
      case KEP_TLS_SIGN:
         x = kep_tls_sign(vec->server_secret, vec->server_len, vec->option, vec->master_secret, out, vec->outlen);
         break;
      case KEP_TLS2_PRF:
         x = kep_tls2_prf(vec->label, vec->labellen, vec->option, vec->master_secret, vec->master_len, vec->server_secret, vec->server_len, out, vec->outlen);
         break;
      case KEP_TLS2_SIGN:
         x = kep_tls2_sign(vec->server_secret, vec->server_len, vec->option, vec->master_secret, vec->master_len, out, vec->outlen);
         break;
      default:
         return 1; // invalid vector opcode
   }
   if (x < 0) {
//      printf("Failed to run KEP vector\n");
      return 1;
   }
   if (memcmp(out, vec->output, vec->outlen)) {
//      printf("Failed to run KEP vector, memcmp\n");
      return 1;
   }
   return 0;
}
