/*
 * Copyright (c) 2013 Elliptic Technologies Inc.
 */
#include "elpkepuser.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

static int do_job(
         uint32_t      opcode,  uint32_t options,
   const unsigned char *label,  uint32_t labellen,
   const unsigned char *master, uint32_t masterlen,
   const unsigned char *server, uint32_t serverlen,
   const unsigned char *client, uint32_t clientlen,
         unsigned char *out,    uint32_t outlen)
{
   int fd;
   unsigned char buf[1024], *p;
   uint32_t x;
   
   if ((7 + labellen + masterlen + serverlen + clientlen) > sizeof(buf)) {
      printf("Invalid buffer lengths\n");
      return -1;
   }
   
   if ((labellen|masterlen|serverlen|clientlen)&~0xFF) {
      printf("No buffer may be bigger than 255 bytes\n");
      return -1;
   }
   
   buf[0] = opcode;
   buf[1] = options;
   buf[2] = masterlen;
   buf[3] = serverlen;
   buf[4] = clientlen;
   buf[5] = labellen;
   buf[6] = outlen>>8;
   buf[7] = outlen&0xFF;
   
   p = &buf[8];
   if (master) { memcpy(p, master, masterlen); p += masterlen; }
   if (server) { memcpy(p, server, serverlen); p += serverlen; }
   if (client) { memcpy(p, client, clientlen); p += clientlen; }
   if (label)  { memcpy(p, label,  labellen);  p += labellen; }
   
   fd = open("/dev/spacckep", O_RDWR);
   if (fd < 0) {
      perror("Cannot open KEP device\n");
      return -1;
   }
   if (write(fd, buf, (p - buf)) != (p - buf)) {
      perror("Cannot write to KEP device\n");
      return -1;
   }
   if ((x = read(fd, out, outlen)) != outlen) {
      printf("Read %zu instead of %zu bytes from KEP\n", x, outlen);
      perror("Cannot read from KEP device\n");
      return -1;
   }
#if 0
{
   int x;
   printf("Read from device\n");
   for (x = 0; x < outlen; ) { printf("%02x", out[x]); if (!(++x & 15)) printf("\n"); }
}
#endif      
   
   close(fd);
   return 0;   
}


int kep_ssl3_keygen(const unsigned char *master_secret,
                    const unsigned char *server_secret,
                    const unsigned char *client_secret,
                          unsigned char *key, uint32_t keylen)
{
   return do_job(KEP_SSL3_KEYGEN, 0, NULL, 0, master_secret, 48, server_secret, 32, client_secret, 32, key, keylen);
}   

int kep_ssl3_sign(const unsigned char *sign_data, uint32_t sign_len, uint32_t options,
                  const unsigned char *master_secret,
                        unsigned char *dgst, uint32_t dgst_len)
{
   return do_job(KEP_SSL3_SIGN, options, NULL, 0, master_secret, 48, sign_data, sign_len, NULL, 0, dgst, dgst_len);
}                
                        
int kep_tls_prf(const unsigned char *label, uint32_t label_len, uint32_t options,
                const unsigned char *master_secret,
                const unsigned char *server_secret, uint32_t server_len,
                      unsigned char *prf, uint32_t prf_len)
{                      
   return do_job(KEP_TLS_PRF, options, label, label_len, master_secret, 48, server_secret, server_len, NULL, 0, prf, prf_len);
}   
  

int kep_tls_sign(const unsigned char *sign_data, uint32_t sign_len, uint32_t options,
                 const unsigned char *master_secret,
                       unsigned char *dgst, uint32_t dgst_len)
{
   return do_job(KEP_TLS_SIGN, options, NULL, 0, master_secret, 48, sign_data, sign_len, NULL, 0, dgst, dgst_len);
}                          


int kep_tls2_prf(const unsigned char *label, uint32_t label_len, uint32_t options,
                 const unsigned char *master_secret, uint32_t master_len,
                 const unsigned char *server_secret, uint32_t server_len,
                       unsigned char *prf, uint32_t prf_len)
{
   return do_job(KEP_TLS2_PRF, options, label, label_len, master_secret, master_len, server_secret, server_len, NULL, 0, prf, prf_len);
}                       

int kep_tls2_sign(const unsigned char *sign_data, uint32_t sign_len, uint32_t options,
                  const unsigned char *master_secret, uint32_t master_len,
                        unsigned char *dgst, uint32_t dgst_len)
{
   return do_job(KEP_TLS2_SIGN, options, NULL, 0, master_secret, master_len, sign_data, sign_len, NULL, 0, dgst, dgst_len);                        
}


