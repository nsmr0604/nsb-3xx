/*
 * Toplevel parser for PKA F/W .sym and .ent files.
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

#include "symbol-scan.h"
#include "symbol-parse.h"
#include "symbol.h"

struct symbol *pka_parse_fw_sym(FILE *f)
{
   struct symbol *syms;
   yyscan_t scanner;
   int rc;

   rc = symbol_yylex_init(&scanner);
   if (rc != 0)
      return NULL;

   symbol_yyset_in(f, scanner);
   rc = symbol_yyparse(scanner, &syms);
   symbol_yylex_destroy(scanner);

   if (rc != 0)
      return NULL;

   return syms;
}
