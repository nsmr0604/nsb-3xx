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


#ifndef _ELPCLUE_H_
#define _ELPCLUE_H_

#define ENCRYPT          0
#define DECRYPT          1

#define CRYPTO_MODULE_CLUE_BN           0x008


//RSA maximum 4096 bit= 512 bytes
#define BN_RSA_MAX_RADIX_SIZE  512
#define BN_RSA_BASE_RADIX_SIZE BN_RSA_MAX_RADIX_SIZE /* XXX: Probably should kill one of these defines */

short clue_bn_modexp (int fd, unsigned char * a, unsigned char * e, unsigned char * m, unsigned char * c, unsigned short size, short precomp);
short clue_bn_modmult (int fd, unsigned char * a, unsigned char * b, unsigned char * m, unsigned char * c, unsigned short size, short precomp);
short clue_bn_moddiv (int fd, unsigned char * a, unsigned char * b, unsigned char * m, unsigned char * c, unsigned short size);
short clue_bn_modinv (int fd, unsigned char * b, unsigned char * m, unsigned char * c, unsigned short size);
short clue_bn_modadd (int fd, unsigned char * a, unsigned char * b, unsigned char * m, unsigned char * c, unsigned short size);
short clue_bn_modsub (int fd, unsigned char * a, unsigned char * b, unsigned char * m, unsigned char * c, unsigned short size);
short clue_bn_modred (int fd, unsigned char * a, unsigned char * m, unsigned char * c, unsigned short size);

int clue_crt_modexp(int fd, const unsigned char *p, const unsigned char *q, const unsigned char *d, const unsigned char *m, unsigned char *c, unsigned size);

void clue_ec_init (void);
int clue_page_size (int size);

short clue_ec_point_mult (int fd, unsigned char * k, unsigned char * x, unsigned char * y, unsigned char * rx, unsigned char * ry, unsigned short size, unsigned short ksize);
short clue_ec_point_mult_base (int fd, unsigned char * k, unsigned char * rx, unsigned char * ry, unsigned short size, unsigned short ksize);
short clue_ec_point_double (int fd, unsigned char * x, unsigned char * y, unsigned char * rx, unsigned char * ry, unsigned short size);
short clue_ec_point_add (int fd, unsigned char * x1, unsigned char * y1, unsigned char * x2, unsigned char * y2, unsigned char * rx, unsigned char * ry, unsigned short size);
short clue_ec_point_verify (int fd, unsigned char * x, unsigned char * y, unsigned short size);

short clue_ec_load_curve_data (int fd, unsigned short curve);
short clue_ec_load_curve_pmult (int fd, unsigned short curve, short loadbase);
short clue_ec_load_curve_pdbl (int fd, unsigned short curve);
short clue_ec_load_curve_padd (int fd, unsigned short curve);
short clue_ec_load_curve_pver (int fd, unsigned short curve);

#endif
