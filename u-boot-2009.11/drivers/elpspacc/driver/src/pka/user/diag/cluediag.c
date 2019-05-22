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
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>

#include <pkadev.h>

#include "elpcrypto.h"

#define CURVE_MAX_DATA_SIZE	256

#define htonl(x) \
({ \
   uint32_t __x = (x); \
   ((uint32_t)( \
      (((uint32_t)(__x) & (uint32_t)0x000000ffUL) << 24) | \
      (((uint32_t)(__x) & (uint32_t)0x0000ff00UL) <<  8) | \
      (((uint32_t)(__x) & (uint32_t)0x00ff0000UL) >>  8) | \
      (((uint32_t)(__x) & (uint32_t)0xff000000UL) >> 24) )); \
})


int _test_invalid_case = 0;
int ecc_diag_verbose = 0;
int ecc_diag_silent = 0;
int fd = 0;




char op_name[][30] = {
  "PMULT",
  "PADD",
  "PVER",
  "PDOUBLE",
  "UNDEF",
  "PMULT(BLINDED)",

  "MODMULT_RAND",
  "MODINV_RNAD",
  "MODEXP_BN",
  "MODMULT_BN",

  "MODDIV_BN",
  "MODADD_BN",
  "MODSUB_BN",
  "MODEXP_CRT",
  "MODINV_BN",

  "MODRED_BN",
  "ECC MODDIV",
  "ECC MODADD",
  "ECC MODSUB",
  "ECC MODRED",

  "PMULT",
  "DSA SIGN",
	"PRECOMPUTE: COMPLETE",
	"PRECOMPUTE: R INV",
	"PRECOMPUTE: MP",
	"PRECOMPUTE: R SQR",
	"MOD_EXP: BASE",
};



/* get a line and skip blanks and comment lines */
static char *get_line (FILE * in)
{
  static char buf[BN_RSA_BASE_RADIX_SIZE];
  char *p;
  unsigned long L;
  do {
    p = fgets (buf, sizeof (buf) - 1, in);
  } while (p && sscanf (buf, "%lx", &L) != 1);
  return p;
}

static void read_words (FILE * in, unsigned char * dst, int nwords)
{
  char *p;
  unsigned long L;
  unsigned long *pdst = (unsigned long *) dst;
  while (nwords-- && (p = get_line (in))) {
    sscanf (p, "%08lx", &L);
    *pdst++ = htonl(L);
  }
}

static int read_params (FILE * in, int *op, int *curve, int *size, int *k_size)
{
  char *p;
  if ((p = get_line (in)) != NULL) {
    sscanf (p, "%d, %d, %d, %d", op, curve, size, k_size);
    return 0;
  }

  return 1;
}


static void outdiff (char *s, unsigned char *buf1, unsigned char *buf2, int len)
{
  int x, y;
  printf ("Comparing [%s]\n", s);
  printf ("\texpect\t -result\n");
  for (x = 0; x < len; x += 4) {
    printf ("%04x   ", x);
    for (y = 0; y < 4; y++)
      printf ("%02lx", (unsigned long) buf1[y] & 255UL);
    printf (" - ");
    for (y = 0; y < 4; y++)
      printf ("%02lx", (unsigned long) buf2[y] & 255UL);
    printf ("\n");
    buf1 += 4;
    buf2 += 4;
  }

}



static int parse_test_script (char *filename)
{
  int failed;
  FILE *in;

  int op, curve, size, k_size, skip = 0;

  struct tm *tm;
  time_t start_time;
  char tbuf[BN_RSA_BASE_RADIX_SIZE];

  unsigned char m[BN_RSA_BASE_RADIX_SIZE];
  unsigned char mp[BN_RSA_BASE_RADIX_SIZE];
  unsigned char a[BN_RSA_MAX_RADIX_SIZE];
  unsigned char b[BN_RSA_MAX_RADIX_SIZE];
  unsigned char x[BN_RSA_MAX_RADIX_SIZE];
  unsigned char y[BN_RSA_MAX_RADIX_SIZE];
  unsigned char e[BN_RSA_MAX_RADIX_SIZE];
  unsigned char d[BN_RSA_MAX_RADIX_SIZE];
  unsigned char k[BN_RSA_MAX_RADIX_SIZE];
  unsigned char rx[BN_RSA_BASE_RADIX_SIZE];
  unsigned char ry[BN_RSA_MAX_RADIX_SIZE];

  if (filename) {
    if (!(in = fopen (filename, "r")))
      return 1;
  } else {
    filename = "<stdin>";
    in = stdin;
  }
#define RI read_int(in)
#define RW(b, l) read_words(in, b, l)

  if (read_params (in, &op, &curve, &size, &k_size) != 0)
    return 2;
  if (ecc_diag_verbose == 1) {
    start_time = time (0);
    tm = localtime (&start_time);
    strftime (tbuf, sizeof (tbuf), "%d-%m-%Y %T", tm);
  }

  memset (a,  0, sizeof a);
  memset (b,  0, sizeof b);
  memset (m,  0, sizeof m);
  memset (e,  0, sizeof e);
  memset (d,  0, sizeof d);
  memset (k,  0, sizeof k);
  memset (mp, 0, sizeof mp);
  memset (y,  0, sizeof y);
  memset (rx, 0, sizeof rx);
  memset (ry, 0, sizeof ry);

  switch (op) {
     case 0:                   // PMULT use internal curve parameters --- G(x,y)
       RW (m, k_size / 4);
       RW (rx, (size+31)/32);
       RW (ry, (size+31)/32);
       break;
     case 1:                   // PADD
       RW (a, (size+31)/32);   //x1
       RW (b, (size+31)/32);   //y1
       RW (m, (size+31)/32);   //x2
       RW (mp, (size+31)/32);  //y2
       RW (rx, (size+31)/32);
       RW (ry, (size+31)/32);
       break;
	 case 2:               // PVER
       RW (rx, (size+31)/32);
       RW (ry, (size+31)/32);
	   break;
     case 3:                   // PDOUBLE
       RW (a, (size+31)/32);   //x
       RW (b, (size+31)/32);   //y
       RW (rx, (size+31)/32);
       RW (ry, (size+31)/32);
       break;

     case 5: // PMULT w/ blinding
       RW (d, (size+31)/32); // blind value
       RW (m, k_size / 4);
       RW (rx, (size+31)/32);
       RW (ry, (size+31)/32);
       break;
       
     case 8:                   // mod_exp
       //software do the precompute, instead of RW (rr, size/32); RW (mp, size/32);
       RW (m, (size+31)/32);
       RW (a, (size+31)/32);
       RW (b, (size+31)/32);
       RW (ry, (size+31)/32);
       break;
     case 9:                   // mod_mult
       //software do the precompute, instead of RW (rr, size/32); RW (mp, size/32);
       RW (m, (size+31)/32);
       RW (a, (size+31)/32);
       RW (b, (size+31)/32);
       RW (ry, (size+31)/32);
       break;
     case 10:                  // mod_div
       RW (m, (size+31)/32);
       RW (a, (size+31)/32);
       RW (b, (size+31)/32);
       RW (ry, (size+31)/32);
       break;
     case 11:                  // mod_add
       RW (m, (size+31)/32);
       RW (a, (size+31)/32);
       RW (b, (size+31)/32);
       RW (ry, (size+31)/32);
       break;
     case 12:                  // mod_sub
       RW (m, (size+31)/32);
       RW (a, (size+31)/32);
       RW (b, (size+31)/32);
       RW (ry, (size+31)/32);
       break;
     case 13: // CRT
       RW (a, (size+63)/64);
       RW (b, (size+63)/64);
       RW (d, (size+31)/32);
       RW (m, (size+31)/32);
       RW (ry, (size+31)/32);
     case 14:                  // mod_inv
       RW (m, (size+31)/32);
       RW (b, (size+31)/32);
       RW (ry, (size+31)/32);
       break;
     case 15:                  // mod_reduction
       RW (m, (size+31)/32);
       RW (a, (size+31)/32);
       RW (ry, (size+31)/32);
       break;
     default:
       break;
  }


  failed = 0;
  switch (op) {
       //ECC
#ifdef DO_CLUE_EC
     case 5:
       if (elppka_set_operand(fd, NULL, "A7", (size+7)/8, d) == -1) {
         failed = 1;
	 perror("failed to load blinding value");
       }

       if (elppka_set_flag(fd, NULL, "F0") == -1) {
         failed = 1;
	 perror("failed to set blinding flag");
       }
       /* fall through to ordinary pmult */
     case 0:
       if ((clue_ec_load_curve_data (fd, curve)) != CRYPTO_OK) {
         failed = 1;
         printf("Failed to load curve data\n");
       }
       if ((clue_ec_load_curve_pmult (fd, curve, 1)) != CRYPTO_OK) {
         failed = 1;
         printf("Failed to load mult data\n");
       }
       if ((clue_ec_point_mult_base (fd, m, x, y, (size+7)/8, k_size)) != CRYPTO_OK)
         failed = 1;
       break;
     case 1:
       if ((clue_ec_load_curve_data (fd, curve)) != CRYPTO_OK)
         failed = 1;
       if ((clue_ec_load_curve_padd (fd, curve)) != CRYPTO_OK)
         failed = 1;
       if ((clue_ec_point_add (fd, a, b, m, mp, x, y, (size+7)/8)) != CRYPTO_OK)
         failed = 1;
       break;
     case 2:
       if ((clue_ec_load_curve_data (fd, curve)) != CRYPTO_OK)
         failed = 1;
       if ((clue_ec_load_curve_pver (fd, curve)) != CRYPTO_OK)
         failed = 1;
       if ((clue_ec_point_verify (fd, rx, ry, (size+7)/8)) != CRYPTO_OK)
         failed = 1;
       break;
     case 3:
       if ((clue_ec_load_curve_data (fd, curve)) != CRYPTO_OK)
         failed = 1;
       if ((clue_ec_load_curve_pdbl (fd, curve)) != CRYPTO_OK)
         failed = 1;
       if ((clue_ec_point_double (fd, a, b, x, y, (size+7)/8)) != CRYPTO_OK)
         failed = 1;
       break;
#endif



     case 8:
       if (clue_bn_modexp (fd, a, b, m, y, (size+7)/8, 1) != CRYPTO_OK)
         failed = 1;
       break;
     case 9:
       if (clue_bn_modmult (fd, a, b, m, y, (size+7)/8, 1) != CRYPTO_OK)
         failed = 1;
       break;
     case 10:
       if (clue_bn_moddiv (fd, a, b, m, y, (size+7)/8) != CRYPTO_OK)
         failed = 1;
       break;
     case 11:
       if (clue_bn_modadd (fd, a, b, m, y, (size+7)/8) != CRYPTO_OK)
         failed = 1;
       break;
     case 12:
       if (clue_bn_modsub (fd, a, b, m, y, (size+7)/8) != CRYPTO_OK)
         failed = 1;
       break;
     case 13:
       if (clue_crt_modexp(fd, a, b, d, m, y, (size+7)/8) != CRYPTO_OK)
         failed = 1;
       break;
     case 14:
       if (clue_bn_modinv (fd, b, m, y, (size+7)/8) != CRYPTO_OK)
         failed = 1;
       break;
     case 15:
       if (clue_bn_modred (fd, a, m, y, (size+7)/8) != CRYPTO_OK)
         failed = 1;
       break;
     default:
       fprintf (stderr, "FAILED: Vector not parsed\n");
       failed = skip = 1;
       break;
  }


  if (!skip) {

    // If failed already do not check the result
    if (!failed) {

      if (op != 2) {
        failed = 0;
        if (memcmp (y, ry, (size+7)/8)) {
          if (!ecc_diag_silent)
            outdiff ("\n\nciphertext Y", ry, y, (size+7)/8);
          failed = 1;
        }

        if ((op < 4 || op == 20 || op == 21) && memcmp (x, rx, (size+7)/8)) {
          if (!ecc_diag_silent)
            outdiff ("ciphertext X", rx, x, (size+7)/8);
          failed = 1;
        }
      }
    }

    if (!ecc_diag_silent)
      fprintf (stderr, "ECC-%s SIZE-%4d %s %s\n", op_name[op], size, filename, failed ? "[FAILED]" : "[PASSED]");
  }


  return failed;
}

int parse_cmd_vectors (int argc, char **argv)
{
  int ret = 0;

  while ((ret = parse_test_script (0)) == 0);
  return (ret == 1) ? 1 : 0;

  if (argc == 1) {
    return parse_test_script (0);
  } else {
    return parse_test_script (argv[0]);
  }
}


int main (int argc, char **argv)
{
  int ret = 0, i;
  int nargc = argc;
  char **nargv = argv;
  char *dev = NULL;


  // skip the program name
  nargc--;
  nargv = (char **) argv[1];

  // added some options and reading stdin stream for redirecting and piping
  // check only two first arguments if any
  // [-v] verbose
  // [-s] silent
  for (i = 1; i <= 10; i++) {
    if (nargc >= 1) {
      if (argv[i][0] == '-') {
        nargc--;
        nargv = (char **) argv[i + 1];
        switch (argv[i][1]) {
           case 'v':
           case 'V':
             ecc_diag_verbose = 1;
           case 's':
           case 'S':
             ecc_diag_silent = 1;
             break;
           case 'I':
           case 'i':
             _test_invalid_case = 1;
             break;
           case 'P':
           case 'p':
             if (argv[i][2] != '=') {
               printf ("SHOULD BE: -p=pka_name\n");
               return -1;
             }
             strcpy (dev, &argv[i][3]);
             break;
        }
      }
    }
  }

  fd = elppka_device_open(dev);
  if (fd == -1) {
    if (dev)
      fprintf(stderr, "%s: %s\n", dev, strerror(errno));
    else
      fprintf(stderr, "%s\n", strerror(errno));
    return EXIT_FAILURE;
  }

#if DO_ECC
  clue_ec_init ();
#endif





  ret = parse_cmd_vectors (nargc, nargv);

  elppka_device_close(fd);

  return ret;
}
