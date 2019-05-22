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

#include "elppka.h"
#include "elppka_hw.h"

#define MAX(a, b) (((a)>(b)) ? (a) : (b))

/*
 * Determine the base radix for the given operand size,
 *   ceiling(lg(size/8))
 * where size > 16 bytes.
 * Returns 0 if the size is invalid.
 */
static unsigned elppka_base_radix(unsigned size)
{
   if (size <= 16)  return 0; /* Error */
   if (size <= 32)  return 2;
   if (size <= 64)  return 3;
   if (size <= 128) return 4;
   if (size <= 256) return 5;
   if (size <= 512) return 6;

   return 0;
}

/*
 * Helper to compute the operand page size, which depends only on the base
 * radix.
 */
static unsigned elppka_page_size(unsigned size)
{
   unsigned ret;

   ret = elppka_base_radix(size);
   if (!ret)
      return ret;
   return 8 << ret;
}

/*
 * Check that the given PKA operand index is valid for a particular bank and
 * operand size.  The bank and size values themselves are not validated.
 */
static int index_is_valid(uint32_t *regs, unsigned bank,
                          unsigned index, unsigned size)
{
   unsigned ecc_max_bytes, rsa_max_bytes, abc_storage, d_storage;
   struct pka_config cfg;
   int rc;

   rc = elppka_get_config(regs, &cfg);
   if (rc != 0)
      return 0;

   ecc_max_bytes = cfg.ecc_size >> 3;
   rsa_max_bytes = cfg.rsa_size >> 3;

   if (size > ecc_max_bytes && size > rsa_max_bytes)
      return 0;
   if (index > 7)
      return 0;

   abc_storage = MAX(ecc_max_bytes*8, rsa_max_bytes*2);
   d_storage   = MAX(ecc_max_bytes*8, rsa_max_bytes*4);

   if (bank == PKA_OPERAND_D) {
      return index < d_storage / size;
   } else {
      return index < abc_storage / size;
   }
}

/*
 * Determine the offset (in 32-bit words) of a particular operand in the PKA
 * memory map.
 * Returns the (non-negative) offset on success, or -errno on failure.
 */
static int operand_base_offset(uint32_t *regs, unsigned bank,
                               unsigned index, unsigned size)
{
   unsigned pagesize;
   int ret;

   pagesize = elppka_page_size(size);
   if (!pagesize)
      return CRYPTO_INVALID_SIZE;

   if (!index_is_valid(regs, bank, index, pagesize))
      return CRYPTO_NOT_FOUND;

   switch (bank) {
   case PKA_OPERAND_A:
      ret = PKA_OPERAND_A_BASE;
      break;
   case PKA_OPERAND_B:
      ret = PKA_OPERAND_B_BASE;
      break;
   case PKA_OPERAND_C:
      ret = PKA_OPERAND_C_BASE;
      break;
   case PKA_OPERAND_D:
      ret = PKA_OPERAND_D_BASE;
      break;
   default:
      return CRYPTO_INVALID_ARGUMENT;
   }

   return ret + index * (pagesize>>2);
}

int elppka_start(uint32_t *regs, uint32_t entry, uint32_t flags, unsigned size)
{
   uint32_t ctrl, base;

   base = elppka_base_radix(size);
   if (!base)
      return CRYPTO_INVALID_SIZE;

   ctrl  = base << PKA_CTRL_BASE_RADIX;
   ctrl |= (size & (size-1) ? (size+3)/4 : 0) << PKA_CTRL_PARTIAL_RADIX;
   ctrl |= 1ul << PKA_CTRL_GO;

   /* Handle ECC-521 oddities as a special case. */
   if (size == PKA_ECC521_OPERAND_SIZE) {
      flags |= 1ul << PKA_FLAG_F1;
      ctrl  |= PKA_CTRL_M521_ECC521 << PKA_CTRL_M521_MODE;
   }

   pdu_io_write32(&regs[PKA_F_STACK], 0);
   pdu_io_write32(&regs[PKA_FLAGS], flags);
   pdu_io_write32(&regs[PKA_ENTRY], entry);
   pdu_io_write32(&regs[PKA_CTRL],  ctrl);

   return 0;
}

void elppka_abort(uint32_t *regs)
{
   pdu_io_write32(&regs[PKA_CTRL], 1 << PKA_CTRL_STOP_RQST);
}

int elppka_get_status(uint32_t *regs, unsigned *code)
{
   uint32_t status = pdu_io_read32(&regs[PKA_RC]);

   if (status & (1 << PKA_RC_BUSY)) {
      return CRYPTO_INPROGRESS;
   }

   if (code) {
      *code = (status >> PKA_RC_REASON) & ((1 << PKA_RC_REASON_BITS)-1);
   }

   return 0;
}

int elppka_load_operand(uint32_t *regs, unsigned bank, unsigned index,
                                        unsigned size, const uint8_t *data)
{
   unsigned i, n;
   uint32_t tmp;
   int opbase;

   opbase = operand_base_offset(regs, bank, index, size);
   if (opbase < 0)
      return opbase;

   regs += opbase;
   n = size >> 2;

   for (i = 0; i < n; i++) {
      /*
       * For lengths that are not a multiple of 4, the incomplete word is
       * at the _start_ of the data buffer, so we must add the remainder.
       */
      memcpy(&tmp, data+((n-i-1)<<2)+(size&3), 4);
      pdu_io_write32(&regs[i], tmp);
   }

   /* Write the incomplete word, if any. */
   if (size & 3) {
      tmp = 0;
      memcpy((char *)&tmp + sizeof tmp - (size&3), data, size & 3);
      pdu_io_write32(&regs[i++], tmp);
   }

   /* Zero the remainder of the operand. */
   for (n = elppka_page_size(size) >> 2; i < n; i++) {
      pdu_io_write32(&regs[i], 0);
   }

   return 0;
}

int elppka_unload_operand(uint32_t *regs, unsigned bank, unsigned index,
                                          unsigned size, uint8_t *data)
{
   unsigned i, n;
   uint32_t tmp;
   int opbase;

   opbase = operand_base_offset(regs, bank, index, size);
   if (opbase < 0)
      return opbase;

   regs += opbase;
   n = size >> 2;

   for (i = 0; i < n; i++) {
      tmp = pdu_io_read32(&regs[i]);
      memcpy(data+((n-i-1)<<2)+(size&3), &tmp, 4);
   }

   if (size & 3) {
      tmp = pdu_io_read32(&regs[i]);
      memcpy(data, (char *)&tmp + sizeof tmp - (size&3), size & 3);
   }

   return 0;
}

/* Parse out the fields from a type-0 BUILD_CONF register in bc. */
static int elppka_get_config_type0(uint32_t bc, struct pka_config *out)
{
   struct pka_config cfg = {0};

   if (bc & (1ul << PKA_BC_FW_HAS_RAM)) {
      cfg.fw_ram_size = 256u << ((bc >> PKA_BC_FW_RAM_SZ)
                                 & ((1ul << PKA_BC_FW_RAM_SZ_BITS)-1));
   }
   if (bc & (1ul << PKA_BC_FW_HAS_ROM)) {
      cfg.fw_rom_size = 256u << ((bc >> PKA_BC_FW_ROM_SZ)
                                 & ((1ul << PKA_BC_FW_ROM_SZ_BITS)-1));
   }

   cfg.alu_size = 32u << ((bc >> PKA_BC_ALU_SZ)
                          & ((1ul << PKA_BC_ALU_SZ_BITS)-1));
   cfg.rsa_size = 512u << ((bc >> PKA_BC_RSA_SZ)
                           & ((1ul << PKA_BC_RSA_SZ_BITS)-1));
   cfg.ecc_size = 256u << ((bc >> PKA_BC_ECC_SZ)
                           & ((1ul << PKA_BC_ECC_SZ_BITS)-1));

   *out = cfg;
   return 0;
}

/* Parse out the fields from a type-1 BUILD_CONF register in bc. */
static int elppka_get_config_type1(uint32_t bc, struct pka_config *out)
{
   struct pka_config cfg = {0};
   uint32_t tmp;

   tmp = (bc >> PKA_BC1_FW_RAM_SZ) & ((1ul << PKA_BC1_FW_RAM_SZ_BITS)-1);
   if (tmp)
      cfg.fw_ram_size = 256u << (tmp-1);

   tmp = (bc >> PKA_BC1_FW_ROM_SZ) & ((1ul << PKA_BC1_FW_ROM_SZ_BITS)-1);
   if (tmp)
      cfg.fw_rom_size = 256u << (tmp-1);

   tmp = (bc >> PKA_BC1_RSA_SZ) & ((1ul << PKA_BC1_RSA_SZ_BITS)-1);
   if (tmp)
      cfg.rsa_size = 512u << (tmp-1);

   tmp = (bc >> PKA_BC1_ECC_SZ) & ((1ul << PKA_BC1_ECC_SZ_BITS)-1);
   if (tmp)
      cfg.ecc_size = 256u << (tmp-1);

   tmp = (bc >> PKA_BC1_ALU_SZ) & ((1ul << PKA_BC1_ALU_SZ_BITS)-1);
   cfg.alu_size = 32u << tmp;

   *out = cfg;
   return 0;
}

int elppka_get_config(uint32_t *regs, struct pka_config *out)
{
   uint32_t bc = pdu_io_read32(&regs[PKA_BUILD_CONF]);

   switch ((bc >> PKA_BC_FORMAT_TYPE) & ((1ul << PKA_BC_FORMAT_TYPE_BITS)-1)) {
   case 0:
      return elppka_get_config_type0(bc, out);
   case 1:
      return elppka_get_config_type1(bc, out);
   }

   return CRYPTO_NOT_IMPLEMENTED;
}

/* Deprecated wrapper around elppka_get_config. */
void elppka_fw_size(uint32_t *regs, unsigned *ramsize, unsigned *romsize)
{
   struct pka_config cfg;
   int rc;

   rc = elppka_get_config(regs, &cfg);
   if (rc != 0)
      return rc;

   if (ramsize)
      *ramsize = cfg.fw_ram_size;
   if (romsize)
      *romsize = cfg.fw_rom_size;
}

void elppka_set_byteswap(uint32_t *regs, int swap)
{
   uint32_t val = pdu_io_read32(&regs[PKA_CONF]);

   if (swap) {
      val |= 1 << PKA_CONF_BYTESWAP;
   } else {
      val &= ~(1 << PKA_CONF_BYTESWAP);
   }

   pdu_io_write32(&regs[PKA_CONF], val);
}
