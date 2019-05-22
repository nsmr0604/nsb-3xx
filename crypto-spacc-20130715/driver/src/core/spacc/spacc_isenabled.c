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

#include "elpspacc.h"

int spacc_isenabled(spacc_device *spacc, int mode, int keysize)
{
   int x;
   static const int keysizes[2][6] = {
      { 5, 8, 16, 24, 32, 0 },     // cipher key sizes
      { 16, 20, 24, 32, 64, 128 },  // hash key sizes
   };

   if (mode < 0 || mode > CRYPTO_MODE_LAST) {
      return 0;
   }

   // always return true for NULL
   if (mode == CRYPTO_MODE_NULL) {
      return 1;
   }

   if (spacc->config.modes[mode] & 128) {
      for (x = 0; x < 6; x++) {
         if (keysizes[1][x] >= keysize && ((1<<x)&spacc->config.modes[mode])) {
            return 1;
         }
      }
      return 0;
   } else {
      for (x = 0; x < 6; x++) {
         if (keysizes[0][x] == keysize) {
            if (spacc->config.modes[mode] & (1<<x)) {
               return 1;
            } else {
               return 0;
            }
         }
      }
   }
   return 0;
}
