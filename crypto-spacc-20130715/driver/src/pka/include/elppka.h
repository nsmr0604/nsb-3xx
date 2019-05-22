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

#ifndef ELPPKA_H_
#define ELPPKA_H_

#include "elppdu.h"

struct pka_config {
   unsigned alu_size, rsa_size, ecc_size;
   unsigned fw_ram_size, fw_rom_size;
};

enum {
   PKA_OPERAND_A,
   PKA_OPERAND_B,
   PKA_OPERAND_C,
   PKA_OPERAND_D,
   PKA_OPERAND_MAX
};

int elppka_start(uint32_t *regs, uint32_t entry, uint32_t flags, unsigned size);
void elppka_abort(uint32_t *regs);
int elppka_get_status(uint32_t *regs, unsigned *code);

int elppka_load_operand(uint32_t *regs, unsigned bank, unsigned index,
                                        unsigned size, const uint8_t *data);
int elppka_unload_operand(uint32_t *regs, unsigned bank, unsigned index,
                                          unsigned size, uint8_t *data);

int elppka_get_config(uint32_t *regs, struct pka_config *cfg);
void elppka_fw_size(uint32_t *regs, unsigned *ramsize, unsigned *romsize);

void elppka_set_byteswap(uint32_t *regs, int swap);

#endif
