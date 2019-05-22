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


#include "elpre.h"
#include "elprehw.h"


int re_error_msg (int err, unsigned char * message, uint32_t length)
{
   unsigned char *msg;
   uint32_t count;

   if (length < 255) {
      return CRYPTO_INVALID_SIZE;
   }

   switch (err) {
      case CRYPTO_OK:
         msg = (unsigned char *) "Operation has succeded";
         break;
      case CRYPTO_FAILED:
         msg = (unsigned char *) "Operation has failed";
         break;
      case CRYPTO_INPROGRESS:
         msg = (unsigned char *) "Operation in progress";
         break;
      case CRYPTO_INVALID_HANDLE:
         msg = (unsigned char *) "Invalid handle";
         break;
      case CRYPTO_INVALID_CONTEXT:
         msg = (unsigned char *) "Invalid context";
         break;
      case CRYPTO_INVALID_SIZE:
         msg = (unsigned char *) "Invalid size";
         break;
      case CRYPTO_NOT_INITIALIZED:
         msg = (unsigned char *) "Crypto library has not been initialized";
         break;
      case CRYPTO_NO_MEM:
         msg = (unsigned char *) "No context memory";
         break;
      case CRYPTO_INVALID_ALG:
         msg = (unsigned char *) "Algorithm is not supported";
         break;
      case CRYPTO_INVALID_IV_SIZE:
         msg = (unsigned char *) "Invalid IV size";
         break;
      case CRYPTO_INVALID_KEY_SIZE:
         msg = (unsigned char *) "Invalid key size";
         break;
      case CRYPTO_INVALID_ARGUMENT:
         msg = (unsigned char *) "Invalid argument";
         break;
      case CRYPTO_MODULE_DISABLED:
         msg = (unsigned char *) "Crypto module disabled";
         break;
      case CRYPTO_NOT_IMPLEMENTED:
         msg = (unsigned char *) "Function is not implemented";
         break;
      case CRYPTO_INVALID_BLOCK_ALIGNMENT:
         msg = (unsigned char *) "Invalid block alignment";
         break;
      case CRYPTO_INVALID_MODE:
         msg = (unsigned char *) "Invalid mode";
         break;
      case CRYPTO_INVALID_KEY:
         msg = (unsigned char *) "Invalid key";
         break;
      case CRYPTO_AUTHENTICATION_FAILED:
         msg = (unsigned char *) "Authentication failed";
         break;
      case CRYPTO_MEMORY_ERROR:
         msg = (unsigned char *) "Internal Memory Error";
         break;
      case CRYPTO_LAST_ERROR:
         msg = (unsigned char *) "Last Error";
         break;
      case CRYPTO_INVALID_ICV_KEY_SIZE:
         msg = (unsigned char *) "Invalid ICV key size";
         break;
      case CRYPTO_INVALID_PARAMETER_SIZE:
         msg = (unsigned char *) "Invalid Parameter size";
         break;
      case CRYPTO_SEQUENCE_OVERFLOW:
         msg = (unsigned char *) "Sequence number overflow";
         break;
      case CRYPTO_DISABLED:
         msg = (unsigned char *) "Read or Write channel is disabled";
         break;
      case CRYPTO_INVALID_VERSION:
         msg = (unsigned char *) "Invalid Version";
         break;
      case CRYPTO_FATAL:
         msg = (unsigned char *) "Fatal Alert detected";
         break;
      case CRYPTO_INVALID_PAD:
         msg = (unsigned char *) "Invalid padding";
         break;
      case CRYPTO_FIFO_FULL:
         msg = (unsigned char *) "Fifo full";
         break;
      case CRYPTO_INVALID_SEQUENCE:
         msg = (unsigned char *) "Invalid Sequence Number length";
         break;
      default:
         msg = (unsigned char *) "Invalid error code";
         break;
   }

   for (count = 0; count < 255; count += 1) {
      if (msg[count] == '\0') {
         message[count] = '\0';
         break;
      }
      message[count] = msg[count];
   }

   return CRYPTO_OK;
}
