%code top {
/*
 * Bison parser for symbol files output by the PKA assembler.
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

%name-prefix "symbol_yy"
%parse-param {void *scanner}
%parse-param {struct symbol **out}
%lex-param {yyscan_t scanner}
%define api.pure
%error-verbose
%locations

%code requires {
#include <inttypes.h>
#include "symbol.h"
}

%code provides {
#ifndef SYMBOL_H_
#define SYMBOL_H_

#define SYMBOL_ERRMSG(loc, msg) do { \
   symbol_yyerror(loc, NULL, NULL, msg); \
} while (0)

void symbol_yyerror(YYLTYPE *, void *, struct symbol **, const char *);
int symbol_yyparse(void *, struct symbol **);

#endif
}

%union {
   struct symbol *symbol;
   uintmax_t number;
   char *string;
}

%{
#include "symbol-scan.h"

#define FAIL(msg) do { \
   SYMBOL_ERRMSG(&yylloc, msg); \
   YYERROR; \
} while (0)

static void symbol_free(struct symbol *sym)
{
   while (sym) {
      struct symbol *tmp = sym->next;
      free(sym);
      sym = tmp;
   }
}

void pka_free_symbols(struct symbol *sym)
{
   symbol_free(sym);
}

%}

%destructor { symbol_free($$); } <symbol>
%destructor { free($$); } <string>

/* Magic token for communicating lexer failures. */
%token T_LEX_ERROR;

%token T_ARROW "=>";
%token <number> T_NUMBER "number";
%token <string> T_IDENTIFIER "identifier";

%type <symbol> symbol symbols

%%

input: symbols {
   *out = $1;
}

symbols: symbol symbols {
   $1->next = $2;
   $$ = $1;
} | { $$ = NULL; }

symbol: T_IDENTIFIER T_ARROW T_NUMBER {
   size_t len = strlen($1);
   $$ = realloc($1, sizeof *$$ + len + 1);
   if (!$$)
      FAIL("failed to allocate memory");

   memmove($$->name, $$, len + 1);
   $$->address = $3;
   $$->next = NULL;
}

%%

void yyerror(YYLTYPE *loc, yyscan_t scanner, struct symbol **out, const char *err)
{
   fprintf(stderr, "%s\n", err);
}
