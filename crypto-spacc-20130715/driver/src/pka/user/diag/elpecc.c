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
#include <pkadev.h>

#include "elpcrypto.h"
#include "elpcurve.h"

#include <errno.h>

#ifdef DO_ECC

#define CURVE_160_DATA_SIZE 20
#define CURVE_160_ORDER_FULL_SIZE 32
#define CURVE_192_DATA_SIZE 24
#define CURVE_192_ORDER_FULL_SIZE 32
#define CURVE_224_DATA_SIZE 28
#define CURVE_224_ORDER_FULL_SIZE 32
#define CURVE_256_DATA_SIZE 32
#define CURVE_256_ORDER_FULL_SIZE 32
#define CURVE_384_DATA_SIZE 48
#define CURVE_384_ORDER_FULL_SIZE 64


#define CLUE_MAX_CURVES 16

#if defined (DO_PKA_EPN0202)
#include "SECP_192.inc"
#include "SECP_224.inc"
#include "SECP_256.inc"
#include "SECP_384.inc"
#elif defined (DO_CLUE_EDN0232)
#include "SECP_160.inc"
#include "SECP_384.inc"
#include "SECP_192.inc"
#include "SECP_224.inc"
#include "SECP_256.inc"
#elif defined (DO_CLUE_EDN160)
#include "SECP_160.inc"
#include "SECP_384.inc"
#include "SECP_192.inc"
#include "SECP_224.inc"
#include "SECP_256.inc"
#elif defined (DO_CLUE_EDN135)
#include "SECP_384.inc"
#elif defined (DO_PKA_EPN1200) 
#include "SECP_160.inc"
#include "SECP_192.inc"
#include "SECP_224.inc"
#include "SECP_256.inc"
#elif defined (DO_PKA_EPN0205) 
#include "SECP_160.inc"
#include "SECP_192.inc"
#include "SECP_224.inc"
#include "SECP_256.inc"
#include "SECP_384.inc"
#elif defined (DO_PKA_EPN0203) 
#include "SECP_160.inc"
#include "SECP_192.inc"
#include "SECP_224.inc"
#include "SECP_256.inc"
#include "SECP_384.inc"
#elif defined (DO_CLUE_ALL) 
#include "SECP_160.inc"
#include "SECP_192.inc"
#include "SECP_224.inc"
#include "SECP_256.inc"
#include "SECP_384.inc"
#include "SECP_512.inc"
#include "SECP_521.inc"
#else
#error "Should define Clue load"
#endif


static EC_CURVE_DATA *curve_data[CLUE_MAX_CURVES];


void clue_ec_init (void)
{
  int i;

  for (i = 0; i < CLUE_MAX_CURVES; i++)
    curve_data[i] = 0;

#if defined (DO_PKA_EPN0202)
  curve_data[4] = (EC_CURVE_DATA *) &SEC192R1;
  curve_data[6] = (EC_CURVE_DATA *) &SEC224R1;
  curve_data[7] = (EC_CURVE_DATA *) &SEC224K1;
  curve_data[8] = (EC_CURVE_DATA *) &SEC256R1;
  curve_data[9] = (EC_CURVE_DATA *) &SEC256K1;
  curve_data[10] = (EC_CURVE_DATA *) &SEC384R1;
#elif defined (DO_CLUE_EDN0232)
  curve_data[3] = (EC_CURVE_DATA *) & SEC384R1;
  curve_data[4] = (EC_CURVE_DATA *) & SEC192R1;
  curve_data[5] = (EC_CURVE_DATA *) & SEC224R1;
  curve_data[6] = (EC_CURVE_DATA *) & SEC256R1;
#elif defined (DO_CLUE_EDN160)
  curve_data[0] = (EC_CURVE_DATA *) & SEC160R1;
  curve_data[1] = (EC_CURVE_DATA *) & SEC160R2;
  curve_data[2] = (EC_CURVE_DATA *) & SEC160K1;
  curve_data[3] = (EC_CURVE_DATA *) & SEC384R1;
  curve_data[4] = (EC_CURVE_DATA *) & SEC192R1;
  curve_data[5] = (EC_CURVE_DATA *) & SEC224R1;
  curve_data[6] = (EC_CURVE_DATA *) & SEC256R1;
#elif defined (DO_CLUE_EDN135)
  curve_data[3] = (EC_CURVE_DATA *) & SEC384R1;
#elif defined (DO_PKA_EPN1200)
  curve_data[0] = (EC_CURVE_DATA *) &SEC160R1;
  curve_data[1] = (EC_CURVE_DATA *) &SEC160R2;
  curve_data[2] = (EC_CURVE_DATA *) &SEC160K1;
  curve_data[3] = (EC_CURVE_DATA *) &SEC160WTLS9;
  curve_data[4] = (EC_CURVE_DATA *) &SEC192R1;
  curve_data[5] = (EC_CURVE_DATA *) &SEC192K1;
  curve_data[6] = (EC_CURVE_DATA *) &SEC224R1;
  curve_data[7] = (EC_CURVE_DATA *) &SEC224K1;
  curve_data[8] = (EC_CURVE_DATA *) &SEC256R1;
  curve_data[9] = (EC_CURVE_DATA *) &SEC256K1;
#elif defined (DO_PKA_EPN0205)
  curve_data[0] = (EC_CURVE_DATA *) &SEC160R1;
  curve_data[1] = (EC_CURVE_DATA *) &SEC160R2;
  curve_data[2] = (EC_CURVE_DATA *) &SEC160K1;
  curve_data[3] = (EC_CURVE_DATA *) &SEC160WTLS9;
  curve_data[4] = (EC_CURVE_DATA *) &SEC192R1;
  curve_data[5] = (EC_CURVE_DATA *) &SEC192K1;
  curve_data[6] = (EC_CURVE_DATA *) &SEC224R1;
  curve_data[7] = (EC_CURVE_DATA *) &SEC224K1;
  curve_data[8] = (EC_CURVE_DATA *) &SEC256R1;
  curve_data[9] = (EC_CURVE_DATA *) &SEC256K1;
  curve_data[10] = (EC_CURVE_DATA *) &SEC384R1;
#elif defined (DO_PKA_EPN0203)
  curve_data[0] = (EC_CURVE_DATA *) &SEC160R1;
  curve_data[1] = (EC_CURVE_DATA *) &SEC160R2;
  curve_data[2] = (EC_CURVE_DATA *) &SEC160K1;
  curve_data[3] = (EC_CURVE_DATA *) &SEC160WTLS9;
  curve_data[4] = (EC_CURVE_DATA *) &SEC192R1;
  curve_data[5] = (EC_CURVE_DATA *) &SEC192K1;
  curve_data[6] = (EC_CURVE_DATA *) &SEC224R1;
  curve_data[7] = (EC_CURVE_DATA *) &SEC224K1;
  curve_data[8] = (EC_CURVE_DATA *) &SEC256R1;
  curve_data[9] = (EC_CURVE_DATA *) &SEC256K1;
  curve_data[10] = (EC_CURVE_DATA *) &SEC384R1;
#elif defined (DO_CLUE_ALL)
  curve_data[0] = (EC_CURVE_DATA *) &SEC160R1;
  curve_data[1] = (EC_CURVE_DATA *) &SEC160R2;
  curve_data[2] = (EC_CURVE_DATA *) &SEC160K1;
  curve_data[3] = (EC_CURVE_DATA *) &SEC160WTLS9;
  curve_data[4] = (EC_CURVE_DATA *) &SEC192R1;
  curve_data[5] = (EC_CURVE_DATA *) &SEC192K1;
  curve_data[6] = (EC_CURVE_DATA *) &SEC224R1;
  curve_data[7] = (EC_CURVE_DATA *) &SEC224K1;
  curve_data[8] = (EC_CURVE_DATA *) &SEC256R1;
  curve_data[9] = (EC_CURVE_DATA *) &SEC256K1;
  curve_data[10] = (EC_CURVE_DATA *) &SEC384R1;
  curve_data[11] = (EC_CURVE_DATA *) &SEC512R1;
  curve_data[12] = (EC_CURVE_DATA *) &SEC521R1;
#else
#error "Should define Clue load"
#endif
}

short clue_ec_load_curve_pmult (int fd, unsigned short curve, short loadbase)
{
  short size;
  unsigned char *x, *y, *a, *b;
  EC_CURVE_DATA *pc = (EC_CURVE_DATA *) curve_data[curve];

  if ((curve > CLUE_MAX_CURVES) || (pc == 0))
    return CRYPTO_INVALID_ARGUMENT;

	size = pc->size;
  x = (unsigned char *) pc->x;
  y = (unsigned char *) pc->y;
  a = (unsigned char *) pc->a;
	b = (unsigned char *) pc->b;

  if (loadbase == 1) 
	{
    //A2 <- x
    if (elppka_set_operand(fd, "", "A2", size, x) != CRYPTO_OK)
      return CRYPTO_FAILED;
    //B2 <- y
    if (elppka_set_operand(fd, "", "B2", size, y) != CRYPTO_OK)
      return CRYPTO_FAILED;
  }
  
	//A6  <- a, (curve)
  if (elppka_set_operand(fd, "", "A6", size, a) != CRYPTO_OK)
    return CRYPTO_FAILED;

  return CRYPTO_OK;
}

short clue_ec_load_curve_pdbl (int fd, unsigned short curve)
{
  short size;
  unsigned char *a;
  EC_CURVE_DATA *pc = (EC_CURVE_DATA *) curve_data[curve];
  unsigned char k[CURVE_MAX_DATA_SIZE];
  unsigned short nextpwr;
  
  if ((curve > CLUE_MAX_CURVES) || (pc == 0))
    return CRYPTO_INVALID_ARGUMENT;

  size = pc->size;
  a = (unsigned char *) pc->a;

  memset (k, 0, sizeof (k));
  k[size - 1] = 1;              // z=1
  //C3  <- z
  if (elppka_set_operand(fd, "", "C3", size, k) != CRYPTO_OK)
     return CRYPTO_FAILED;
     
  for (nextpwr = 1; nextpwr < size; nextpwr *= 2);
   
  k[size - 1] = 0;
  k[nextpwr - 1] = 2; // K=2
  //D7  <- k, key (2<key<order of base point)
  if (elppka_set_operand(fd, "", "D7", size, k) != CRYPTO_OK)
     return CRYPTO_FAILED;

  //A6  <- a, (curve)
  if (elppka_set_operand(fd, "", "A6", size, a) != CRYPTO_OK)
     return CRYPTO_FAILED;
  //PDUMPWORD (EDDUMP, p, size, "//a", 1);

  return CRYPTO_OK;
}

short clue_ec_load_curve_padd (int fd, unsigned short curve)
{
  short size;
  unsigned char *a;
  EC_CURVE_DATA *pc = (EC_CURVE_DATA *) curve_data[curve];

  if ((curve > CLUE_MAX_CURVES) || (pc == 0))
    return CRYPTO_INVALID_ARGUMENT;

  size = pc->size;
  a = (unsigned char *) pc->a;

  //A6  <- a, (curve)
  if (elppka_set_operand(fd, "", "A6", size, a) != CRYPTO_OK)
     return CRYPTO_FAILED;

  return CRYPTO_OK;
}

short clue_ec_load_curve_pver (int fd, unsigned short curve)
{
  short size;
  unsigned char *a, *b;
  EC_CURVE_DATA *pc = (EC_CURVE_DATA *) curve_data[curve];

  if ((curve > CLUE_MAX_CURVES) || (pc == 0))
     return CRYPTO_INVALID_ARGUMENT;

  size = pc->size;
  a = (unsigned char *) pc->a;
  b = (unsigned char *) pc->b;


  // A6 <- a,(curve)
  if (elppka_set_operand(fd, "", "A6", size, a) != CRYPTO_OK)
     return CRYPTO_FAILED;
  // A7 <- b,(curve)
  if (elppka_set_operand(fd, "", "A7", size, b) != CRYPTO_OK)
     return CRYPTO_FAILED;

  return CRYPTO_OK;
}


short clue_ec_load_curve_data (int fd, unsigned short curve)
{
  short size;
  unsigned char *m, *mp, *r;
  EC_CURVE_DATA *pc = (EC_CURVE_DATA *) curve_data[curve];

  if ((curve > CLUE_MAX_CURVES) || (pc == 0))
    return CRYPTO_INVALID_ARGUMENT;
  size = pc->size;
  m = (unsigned char *) pc->m;
  mp = (unsigned char *) pc->mp;
  r = (unsigned char *) pc->r;

  //D0  <- modulus m (p)
  if (elppka_set_operand(fd, "", "D0", size, m) != CRYPTO_OK)
     return CRYPTO_FAILED;
  //D1  <- m' ,modular inverse of m (mod R)
  if (elppka_set_operand(fd, "", "D1", size, mp) != CRYPTO_OK)
     return CRYPTO_FAILED;
  //D3  <- r_sqr_mod_m
  if (elppka_set_operand(fd, "", "D3", size, r) != CRYPTO_OK)
     return CRYPTO_FAILED;


  return CRYPTO_OK;
}

#if 0 // this is untested but should work
/////////////////////////////////////////////// clue ecc
//point multiplication: k*P(x,y)
//intputs:      k, key
//            x, x of P(x,y)  point on the curve
//    y, y of P(x,y)
//output:   rx
//        ry
short clue_ec_point_mult (int fd, unsigned char * k, unsigned char * x, unsigned char * y, unsigned char * rx, unsigned char * ry, unsigned short size, unsigned short ksize)
{
  short ret = CRYPTO_OK;
  unsigned char kbuf[CURVE_MAX_DATA_SIZE];
  unsigned short nextpwr;
  
  for (nextpwr = 1; nextpwr < size; nextpwr *= 2);

  if ((k == 0) || (rx == 0) || (ry == 0) || (ksize < 2) || (ksize > CURVE_MAX_DATA_SIZE)) {
    ret = CRYPTO_INVALID_ARGUMENT;
  } else if ((x == 0) || (y == 0)) {
    ret = CRYPTO_INVALID_ARGUMENT;
  } else {
    //load data

    //D7  <- k, key (2<key<order of base point), must full size, 48+16=64 bytes
    if (k) {
       memset (kbuf, 0, sizeof(kbuf));
       memcpy (kbuf + nextpwr - ksize, k, ksize);
       if (elppka_set_operand(fd, "", "D7", nextpwr, kbuf) != CRYPTO_OK){
         return CRYPTO_FAILED;
       }
    }

    if (elppka_run(fd, "pmult", size,
               "%A2",  x,
               "%B2",  y,
               "%A3",  x,
               "%B3",  y,
               "=%A2", rx,
               "=%B2", ry,
               (char *)NULL) != CRYPTO_OK) {
      ret = CRYPTO_FAILED;
    }
    
  }
  return ret;
}
#endif


/////////////////////////////////////////////// clue ecc
//point multiplication: k*G(x,y)
//intputs:      k, key
//            x, x of G(x,y)  base point
//    y, y of G(x,y)
//output:   rx
//        ry
short clue_ec_point_mult_base (int fd, unsigned char * k, unsigned char * rx, unsigned char * ry, unsigned short size, unsigned short ksize)
{
  short ret = CRYPTO_OK;
  unsigned char kbuf[CURVE_MAX_DATA_SIZE];
  unsigned short nextpwr;
  
  for (nextpwr = 1; nextpwr < size; nextpwr *= 2);


  if ((rx == 0) || (ry == 0) || (ksize < 2) || (ksize > CURVE_MAX_DATA_SIZE)) {
    ret = CRYPTO_INVALID_ARGUMENT;
  } else {
  
    //D7  <- k, key (2<key<order of base point), must full size, 48+16=64 bytes
    if (k) {
       memset (kbuf, 0, sizeof(kbuf));
       memcpy (kbuf + nextpwr - ksize, k, ksize);
       if (elppka_set_operand(fd, "", "D7", nextpwr, kbuf) != CRYPTO_OK){
         return CRYPTO_FAILED;
       }
    }

    if ((ret= elppka_run(fd, "pmult", size,
               "=%A2", rx,
               "=%B2", ry,
               (char *)NULL)) != CRYPTO_OK) {
      ret = CRYPTO_FAILED;
    }
  }
  return ret;
}

//point double: k*P(x,y), where k=2
//intputs:
//          x, x of P(x,y)
//    y, y of P(x,y)
//output: rx
//    ry
short clue_ec_point_double (int fd, unsigned char * x, unsigned char * y, unsigned char * rx, unsigned char * ry, unsigned short size)
{
  short ret = CRYPTO_OK;


  if ((x == 0) || (y == 0) || (rx == 0) || (ry == 0)) {
    ret = CRYPTO_INVALID_ARGUMENT;
  } else {
    if (elppka_run(fd, "pdbl", size,
               "%A3",  x,
               "%B3",  y,
               "=%A2", rx,
               "=%B2", ry,
               (char *)NULL) != CRYPTO_OK) {
      ret = CRYPTO_FAILED;
    }
  }
  return ret;
}

//point addition: // P(x1,y1)+Q(x2,y2), where k=1
//intputs:
//          x1, x1 of P(x1,y1)
//    y1, y1 of P(x1,y1)
//          x2, x2 of Q(x2,y2)
//    y2, y2 of Q(x2,y2)
//output: rx
//    ry
short clue_ec_point_add (int fd, unsigned char * x1, unsigned char * y1, unsigned char * x2, unsigned char * y2, unsigned char * rx, unsigned char * ry,
                       unsigned short size)
{
  short ret = CRYPTO_OK;

  if ((x1 == 0) || (y1 == 0) || (x2 == 0) || (y2 == 0) || (rx == 0)
      || (ry == 0)) {
    ret = CRYPTO_INVALID_ARGUMENT;
  } else {
    if (elppka_run(fd, "padd", size,
               "%A2",  x1,
               "%B2",  y1,
               "%A3",  x2,
               "%B3",  y2,
               "=%A2",  rx,
               "=%B2",  ry,
               (char *)NULL) != CRYPTO_OK) {
      ret = CRYPTO_FAILED;
    }
  }
  return ret;
}

//point verify: verify if P(x,y) is on EC y^2=x^2 + ax + b
//intputs:
//        x, x of P(x,y)
//        y, y of P(x,y)
//output: zero flag set if lhs=rhs
short clue_ec_point_verify (int fd, unsigned char * x, unsigned char * y, unsigned short size)
{
  short ret = CRYPTO_OK;
  short zflag;
  
  if ((x == 0) || (y == 0)) {
    ret = CRYPTO_INVALID_ARGUMENT;
  } else {
    if (elppka_run(fd, "pver", size,
               "%A2",  x,
               "%B2",  y,
              // "=%B2",  ry,
               (char *)NULL) != CRYPTO_OK) {
      ret = CRYPTO_FAILED;
    }

	zflag = elppka_test_flag(fd,"","Z");
	if (zflag) {
	   ret = CRYPTO_OK;
	}
	else {
	   ret = CRYPTO_FAILED;
	}
  }
  return ret;
}

#endif /* DO_ECC */
