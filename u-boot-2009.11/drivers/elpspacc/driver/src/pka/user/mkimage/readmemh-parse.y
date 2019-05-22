%code top {
/*
 * Bison parser for IEEE-1364 $readmemh-style input files.
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
}

%name-prefix "readmemh_yy"
%parse-param {void *scanner}
%parse-param {struct readmemh_addrinfo *addr}
%parse-param {uintmax_t *valout}
%parse-param {uintmax_t *xout}
%parse-param {uintmax_t *zout}
%lex-param {yyscan_t scanner}
%define api.pure
%error-verbose
%locations

%code requires {
#include <inttypes.h>
}

%code provides {
#ifndef READMEMH_H_
#define READMEMH_H_

struct readmemh_addrinfo {
   uintmax_t min, max, current;
};

#define READMEMH_ERRMSG(loc, msg) do { \
   readmemh_yyerror(loc, NULL, NULL, NULL, NULL, NULL, msg); \
} while (0)

void readmemh_yyerror(YYLTYPE *, void *, struct readmemh_addrinfo *, uintmax_t *, uintmax_t *, uintmax_t *, const char *);
int readmemh_yyparse(void *, struct readmemh_addrinfo *, uintmax_t *, uintmax_t *, uintmax_t *);

#endif
}

%union {
   struct { uintmax_t val, xmask, zmask; } number;
}

%{
#include "readmemh-scan.h"

#define FAIL(msg) do { \
   READMEMH_ERRMSG(&yylloc, msg); \
   YYERROR; \
} while (0)
%}

/* Magic token for communicating lexer failures. */
%token T_LEX_ERROR;

%token <number> T_NUMBER;
%token <number> T_ADDRESS;

%%

input: | input T_NUMBER {
   if (addr->min <= addr->current && addr->current <= addr->max) {
      uintmax_t i = addr->current - addr->min;

      valout[i] = (valout[i] & $2.xmask) | ($2.val & ~$2.xmask);
      if (xout) {
         xout[i] = $2.xmask;
      }

      if (zout) {
         zout[i] = $2.xmask;
      } else if ($2.zmask) {
         FAIL("tristate input not supported");
      }

      addr->current++;
   } else {
      READMEMH_ERRMSG(&yylloc, "memory value out-of-bounds");
   }
} | input T_ADDRESS {
   addr->current = $2.val;
}

%%

void yyerror(YYLTYPE *loc, yyscan_t scanner, struct readmemh_addrinfo *addr, uintmax_t *valout, uintmax_t *xout, uintmax_t *zout, const char *err)
{
   fprintf(stderr, "%s\n", err);
}
