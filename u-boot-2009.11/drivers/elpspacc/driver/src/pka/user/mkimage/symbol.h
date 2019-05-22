#ifndef PKA_SYMBOL_H_
#define PKA_SYMBOL_H_

#include <stdio.h>
#include <stdint.h>

struct symbol {
   struct symbol *next;
   uintmax_t address;
   char name[];
};

/*
 * Read symbol input from the specified file handle, returning a pointer to
 * the first symbol in a linked list of symbols.  The resulting list can be
 * freed by calling pka_free_symbols.
 *
 * Returns NULL on failure.
 */
struct symbol *pka_parse_fw_sym(FILE *f);
void pka_free_symbols(struct symbol *sym);

#endif
