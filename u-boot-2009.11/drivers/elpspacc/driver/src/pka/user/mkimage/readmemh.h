#ifndef PKA_READMEMH_H_
#define PKA_READMEMH_H_

#include <stdio.h>

#define PKA_MAX_FW_WORDS 2048

/*
 * Read .hex input from the specified file handle, storing the code in
 * fw_out which must be large enough to store MAX_FW_WORDS (2048) instructions.
 *
 * On success, returns the index of the highest valid output instruction,
 * plus one.  Returns -1 on failure.
 */
int pka_parse_fw_hex(FILE *f, unsigned long *fw_out);

#endif
