%top{
/*
 * Flex scanner for IEEE-1364 $readmemh-style input files.
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
#include "readmemh-parse.h"
}

%option nodefault bison-locations reentrant never-interactive
%option noyywrap nounput noinput
%option prefix="readmemh_yy"

%x COMMENT

%{
#define FAIL(msg) do { \
   READMEMH_ERRMSG(yylloc, msg); \
   return T_LEX_ERROR; \
} while(0)

static int decode_hex(int c)
{
   switch (c) {
   case 'a': case 'A': return 0xa;
   case 'b': case 'B': return 0xb;
   case 'c': case 'C': return 0xc;
   case 'd': case 'D': return 0xd;
   case 'e': case 'E': return 0xe;
   case 'f': case 'F': return 0xf;
   }

   return c - '0';
}
%}

%%

"/*" BEGIN(COMMENT);
<COMMENT>{
   "*/" BEGIN(INITIAL);
   \n
   .
}
<INITIAL>"//".*$

[_[:xdigit:]xXzZ]+ { 
   yylval->number.val = yylval->number.zmask = 0;
   yylval->number.xmask = -1;

   for (char *c = yytext; *c; c++) {
      if (*c == '_')
         continue;

      if (yylval->number.val > UINTMAX_MAX/16
          || yylval->number.zmask > UINTMAX_MAX/16) {
         FAIL("input value is too large");
      }

      yylval->number.val *= 16;
      yylval->number.xmask *= 16;
      yylval->number.zmask *= 16;

      switch (*c) {
      case 'x': case 'X': yylval->number.xmask |= 0xf; break;
      case 'z': case 'Z': yylval->number.zmask |= 0xf; break;
      default: yylval->number.val += decode_hex(*c);
      }
   }

   return T_NUMBER;
}

@[[:xdigit:]]+ {
   yylval->number.val = yylval->number.xmask = yylval->number.zmask = 0;

   for (char *c = yytext+1; *c; c++) {
      if (yylval->number.val > UINTMAX_MAX/16)
         FAIL("input address is too large");

      yylval->number.val *= 16;
      yylval->number.val += decode_hex(*c);
   }

   return T_ADDRESS;
}

[[:space:]]+
. {
   char buf[] = "syntax error, unexpected #";
   *strchr(buf, '#') = *yytext;
   FAIL(buf);
}

%%
