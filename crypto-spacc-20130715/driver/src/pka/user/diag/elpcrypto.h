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


#ifndef _ELPCRYPTO_H_
#define _ELPCRYPTO_H_

#define CRYPTO_OK                       (int)0
#define CRYPTO_FAILED                   (int)-1
#define CRYPTO_INPROGRESS               (int)-2
#define CRYPTO_INVALID_HANDLE           (int)-3
#define CRYPTO_INVALID_CONTEXT          (int)-4
#define CRYPTO_INVALID_SIZE             (int)-5
#define CRYPTO_NOT_INITIALIZED          (int)-6
#define CRYPTO_NO_MEM                   (int)-7
#define CRYPTO_INVALID_ALG              (int)-8
#define CRYPTO_INVALID_KEY_SIZE         (int)-9
#define CRYPTO_INVALID_ARGUMENT         (int)-10
#define CRYPTO_MODULE_DISABLED          (int)-11
#define CRYPTO_NOT_IMPLEMENTED          (int)-12
#define CRYPTO_INVALID_BLOCK_ALIGNMENT  (int)-13
#define CRYPTO_INVALID_MODE             (int)-14
#define CRYPTO_INVALID_KEY              (int)-15
#define CRYPTO_AUTHENTICATION_FAILED    (int)-16
#define CRYPTO_INVALID_IV_SIZE          (int)-17
#define CRYPTO_MEMORY_ERROR             (int)-18
#define CRYPTO_LAST_ERROR               (int)-19
#define CRYPTO_HALTED                   (int)-20
#define CRYPTO_TIMEOUT                  (int)-21
#define CRYPTO_SRM_FAILED               (int)-22

#include "elpclue.h"

#include <string.h>
#include <stdio.h>

#endif
