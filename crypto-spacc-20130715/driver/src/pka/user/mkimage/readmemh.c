/*
 * Toplevel parser for PKA F/W .hex files (IEEE-1364 $readmemh-style input)
 *
 * Copyright (C) 2013 Elliptic Technologies Inc.
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>

#include "readmemh-parse.h"
#include "readmemh-scan.h"
#include "readmemh.h"

int pka_parse_fw_hex(FILE *f, unsigned long *fw_out)
{
   uintmax_t fw_data[PKA_MAX_FW_WORDS], fw_xmask[PKA_MAX_FW_WORDS];
   struct readmemh_addrinfo addr = { .max = PKA_MAX_FW_WORDS };
   int rc, top_word = 0;
   yyscan_t scanner;

   for (size_t i = 0; i < sizeof fw_xmask / sizeof fw_xmask[0]; i++) {
      fw_xmask[i] = -1;
   }

   rc = readmemh_yylex_init(&scanner);
   if (rc != 0)
      return -1;

   readmemh_yyset_in(f, scanner);
   rc = readmemh_yyparse(scanner, &addr, fw_data, fw_xmask, NULL);
   readmemh_yylex_destroy(scanner);

   if (rc != 0)
      return -1;

   for (size_t i = 0; i < sizeof fw_data / sizeof fw_data[0]; i++) {
      if ((fw_data[i] & ~fw_xmask[i]) > 0xffffffff) {
         return -1;
      }

      fw_out[i] = fw_data[i] & ~fw_xmask[i];
      if (fw_xmask[i] != -1)
         top_word = i;
   }

   return top_word+1;
}
