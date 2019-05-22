#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/resource.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include "elpspaccusr.h"

struct list { char *name; int val; };

struct list ciphers[] = {
     { "null",     CRYPTO_MODE_NULL     },
     { "descbc",   CRYPTO_MODE_DES_CBC  },
     { "3descbc",  CRYPTO_MODE_3DES_CBC },
     { "aesctr",   CRYPTO_MODE_AES_CTR  },
     { "aescbc",   CRYPTO_MODE_AES_CBC  },
     { "aesccm",   CRYPTO_MODE_AES_CCM  },
     { "aesgcm",   CRYPTO_MODE_AES_GCM  },
     { "rc4128",   CRYPTO_MODE_RC4_128  },
     { "rc440",    CRYPTO_MODE_RC4_40   },
     { NULL, 0 },
};

struct list hashes[] = {
   { "null",       CRYPTO_MODE_NULL },
   { "md5hmac",    CRYPTO_MODE_HMAC_MD5 },
   { "sha1hmac",   CRYPTO_MODE_HMAC_SHA1 },
   { "sha256hmac", CRYPTO_MODE_HMAC_SHA256 },
   { "sha512hmac", CRYPTO_MODE_HMAC_SHA512 },
   { NULL, 0 }
};

uint32_t find_list(struct list *foo, char *str)
{
   uint32_t x;
   for (x = 0; foo[x].name != NULL; x++) {
      if (!strcmp(foo[x].name, str)) {
         fprintf(stderr, "Picked %s\n", str);
         return foo[x].val;
      }
   }
   fprintf(stderr, "Invalid option [%s]...\n", str);
   return 0;
}

struct thread
{
   int size;
   int cmode, hmode;
   int packets;
   int ckeylen, hkeylen;
   int inplace;
};

void * speed(void *d)
{
   struct elp_spacc_usr fd;
   struct thread *jd = d;
   unsigned char *buf[2], blah[64];
   unsigned x, y, preaad, outlen;

   buf[0] = malloc(jd->size+128);
   buf[1] = malloc(jd->size+128);

   if (jd->cmode == CRYPTO_MODE_AES_CCM) {
      preaad = 32;
   } else {
      preaad = 0;
   }

   x = spacc_dev_open(&fd, jd->cmode, jd->hmode, 1, 0, 0, 0, blah, jd->ckeylen, blah, 16, blah, jd->hkeylen, blah, 16);
   assert(!x);

   outlen = jd->size + fd.io.icv_len;

   if (jd->inplace) {
      for (x = 0; x < jd->packets; x++) {
         y = spacc_dev_process(&fd, NULL, -1, preaad, 0, 0, 0, buf[0], jd->size, buf[0], jd->size+128);
         assert(y == outlen);
         if (fd.io.err != 0) {fprintf(stderr, "FAILED: ERR == %d\n", fd.io.err); }
         assert(fd.io.err == 0);
      }
   } else {
      for (x = 0; x < jd->packets; x++) {
         y = spacc_dev_process(&fd, NULL, -1, preaad, 0, 0, 0, buf[0], jd->size, buf[1], jd->size+128);
         assert(y == outlen);
         if (fd.io.err != 0) {fprintf(stderr, "FAILED: ERR == %d\n", fd.io.err); }
         assert(fd.io.err == 0);
      }
   }
   spacc_dev_close(&fd);
   free(buf[0]);
   free(buf[1]);
   return NULL;
}

int main(int argc, char **argv)
{
   int threads, x, payloadsize;
   pthread_t pt[32];
   struct rusage usage;
   double stime, utime, wtime, jobs_per_second, percent_cpu, jobs_per_percent, throughput, npackets, bits_processed;
   struct timeval t1, t2;
   struct thread jd;

   // parse options
   threads     = 1;
   payloadsize = 4096;
   npackets    = 32768.0;

   memset(&jd, 0, sizeof jd);

   jd.ckeylen = 16;
   jd.hkeylen = 16;
   jd.cmode   = CRYPTO_MODE_AES_CBC;
   jd.hmode   = CRYPTO_MODE_HMAC_SHA1;
   jd.inplace = 1;

   for (x = 1; x < argc; x++) {
      if (!strcmp(argv[x], "--size"))       { payloadsize  = atoi(argv[x+1]); ++x; }
      if (!strcmp(argv[x], "--cipher"))     { jd.cmode     = find_list(ciphers, argv[x+1]); ++x; }
      if (!strcmp(argv[x], "--ciphersize")) { jd.ckeylen   = atoi(argv[x+1]); ++x; }
      if (!strcmp(argv[x], "--hash"))       { jd.hmode     = find_list(hashes,  argv[x+1]); ++x; }
      if (!strcmp(argv[x], "--hashsize"))   { jd.hkeylen   = atoi(argv[x+1]); ++x; }
      if (!strcmp(argv[x], "--runs"))       { npackets     = strtoul(argv[x+1], NULL, 10); ++x; }
      if (!strcmp(argv[x], "--threads"))    { threads      = strtoul(argv[x+1], NULL, 10); ++x; }
      if (!strcmp(argv[x], "--inplace"))    { jd.inplace   = strtoul(argv[x+1], NULL, 10); ++x; }
   }

   if (threads > 32 || threads < 1) {
      threads = 1;
   }

   jd.size    = payloadsize;
   jd.packets = npackets;

   // initial time
   gettimeofday(&t1, NULL);

   // run test
   for (x = 0; x < threads; x++) {
      pthread_create(&pt[x], NULL, &speed, &jd);
   }
   for (x = 0; x < threads; x++) {
      pthread_join(pt[x], NULL);
   }

   // output stats
   getrusage(RUSAGE_SELF, &usage);
   gettimeofday(&t2, NULL);
   bits_processed   = threads * npackets * 8.0 * payloadsize;
   wtime            = ((t2.tv_sec*1000000+t2.tv_usec) - (t1.tv_sec*1000000+t1.tv_usec))/1000000.0;  // times are in seconds
   stime            = (usage.ru_stime.tv_sec*1000000+usage.ru_stime.tv_usec)/1000000.0;
   utime            = (usage.ru_utime.tv_sec*1000000+usage.ru_utime.tv_usec)/1000000.0;
   jobs_per_second  = (npackets * threads) / wtime;
   percent_cpu      = ((stime + utime)/wtime)*100.0;
   jobs_per_percent = jobs_per_second / percent_cpu;
   throughput       = bits_processed / (wtime * 1000000.0);
   printf("%d,%3.5f,%3.5f,%3.5f,%2.3f,%4.2f,%.2f,%.2f\n",
      threads,           // # of threads
      wtime,             // wall time
      utime,             // user time
      stime,             // sys time
      percent_cpu,       // % of wall time
      throughput,        // bits per microsecond (megabit/sec)
      jobs_per_second,   // jobs per second
      jobs_per_percent   // jobs per second per 1% of CPU
   );



}
