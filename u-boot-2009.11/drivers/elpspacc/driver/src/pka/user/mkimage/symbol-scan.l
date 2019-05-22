%top{
/*
 * Flex scanner for symbol files output by the PKA assembler.
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
#include "symbol-parse.h"
}

%option nodefault bison-locations reentrant never-interactive
%option noyywrap nounput noinput
%option prefix="symbol_yy"

%s RHS

%{
#define FAIL(msg) do { \
   SYMBOL_ERRMSG(yylloc, msg); \
   return T_LEX_ERROR; \
} while(0)
%}

%%

"#".*$

<RHS>\n BEGIN(INITIAL);
<RHS>[[:xdigit:]]+ {
   char *end;

   errno = 0;
   yylval->number = strtoumax(yytext, &end, 16);
   if (errno == ERANGE)
      FAIL("input value is too large");
   if (*end)
      FAIL("invalid integer input");

   return T_NUMBER;
}

[_[:alpha:]][_[:alnum:]]+ {
   yylval->string = malloc(yyleng+1);
   if (!yylval->string)
      FAIL("failed to allocate memory");
   strcpy(yylval->string, yytext);
   return T_IDENTIFIER;
}

"=>" {
   BEGIN(RHS);
   return T_ARROW;
}

[[:space:]]+
. {
   char buf[] = "syntax error, unexpected #";
   *strchr(buf, '#') = *yytext;
   FAIL(buf);
}

%%
