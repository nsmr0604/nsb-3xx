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

#include <linux/elf.h>
#include <linux/errno.h>
#include <asm/byteorder.h>

#include "firmware.h"
#include "elppka.h"

static void copy_ehdr(Elf32_Ehdr *ehdr, const void *buf)
{
   memcpy(ehdr, buf, sizeof *ehdr);

   ehdr->e_type      = le16_to_cpu(ehdr->e_type);
   ehdr->e_machine   = le16_to_cpu(ehdr->e_machine);
   ehdr->e_version   = le32_to_cpu(ehdr->e_version);
   ehdr->e_entry     = le32_to_cpu(ehdr->e_entry);
   ehdr->e_phoff     = le32_to_cpu(ehdr->e_phoff);
   ehdr->e_shoff     = le32_to_cpu(ehdr->e_shoff);
   ehdr->e_flags     = le32_to_cpu(ehdr->e_flags);
   ehdr->e_ehsize    = le16_to_cpu(ehdr->e_ehsize);
   ehdr->e_phentsize = le16_to_cpu(ehdr->e_phentsize);
   ehdr->e_phnum     = le16_to_cpu(ehdr->e_phnum);
   ehdr->e_shentsize = le16_to_cpu(ehdr->e_shentsize);
   ehdr->e_shnum     = le16_to_cpu(ehdr->e_shnum);
   ehdr->e_shstrndx  = le16_to_cpu(ehdr->e_shstrndx);
}

static void copy_shdr(Elf32_Shdr *shdr, const void *buf)
{
   memcpy(shdr, buf, sizeof *shdr);

   shdr->sh_name      = le32_to_cpu(shdr->sh_name);
   shdr->sh_type      = le32_to_cpu(shdr->sh_type);
   shdr->sh_flags     = le32_to_cpu(shdr->sh_flags);
   shdr->sh_addr      = le32_to_cpu(shdr->sh_addr);
   shdr->sh_offset    = le32_to_cpu(shdr->sh_offset);
   shdr->sh_size      = le32_to_cpu(shdr->sh_size);
   shdr->sh_link      = le32_to_cpu(shdr->sh_link);
   shdr->sh_info      = le32_to_cpu(shdr->sh_info);
   shdr->sh_addralign = le32_to_cpu(shdr->sh_addralign);
   shdr->sh_entsize   = le32_to_cpu(shdr->sh_entsize);
}

static void copy_sym(Elf32_Sym *sym, const void *buf)
{
   memcpy(sym, buf, sizeof *sym);

   sym->st_name  = le32_to_cpu(sym->st_name);
   sym->st_value = le32_to_cpu(sym->st_value);
   sym->st_size  = le32_to_cpu(sym->st_size);
   sym->st_shndx = le16_to_cpu(sym->st_shndx);
}

int pka_fw_init(struct pka_fw *fw, const unsigned char *data,
                                   unsigned long len)
{
   static const unsigned char expected_magic[EI_NIDENT] = {
      [EI_MAG0]    = ELFMAG0,
      [EI_MAG1]    = ELFMAG1,
      [EI_MAG2]    = ELFMAG2,
      [EI_MAG3]    = ELFMAG3,
      [EI_CLASS]   = ELFCLASS32,
      [EI_DATA]    = ELFDATA2LSB,
      [EI_VERSION] = EV_CURRENT,
      [EI_OSABI]   = 255, /* ELFOSABI_STANDALONE */
   };

   Elf32_Ehdr ehdr;

   if (len < sizeof ehdr)
      return CRYPTO_INVALID_FIRMWARE;
   copy_ehdr(&ehdr, data);

   /* Verify ELF header sanity. */
   if (memcmp(ehdr.e_ident, expected_magic, sizeof expected_magic)
       || ehdr.e_machine != 0xe117
       || ehdr.e_type != ET_EXEC
       || (ehdr.e_shnum && ehdr.e_shnum <= ehdr.e_shstrndx)
       || ehdr.e_shentsize != sizeof (Elf32_Shdr)) {
      return CRYPTO_INVALID_FIRMWARE;
   }

   /* Verify that section header table fits in the data. */
   if (len < ehdr.e_shoff)
      return CRYPTO_INVALID_FIRMWARE;
   if (len - ehdr.e_shoff < (unsigned long)ehdr.e_shentsize * ehdr.e_shnum)
      return CRYPTO_INVALID_FIRMWARE;

   memset(fw, 0, sizeof *fw);
   fw->ehdr = ehdr;
   fw->base = data;
   fw->len  = len;

   return 0;
}

int pka_fw_read_shdr(struct pka_fw *fw, unsigned long index, Elf32_Shdr *shdr)
{
   Elf32_Off pos = fw->ehdr.e_shoff;

   if (le16_to_cpu(fw->ehdr.e_shnum) <= index)
      return CRYPTO_INVALID_ARGUMENT;

   pos += index * fw->ehdr.e_shentsize;
   copy_shdr(shdr, fw->base + pos);

   /* Alignment of 0 means the same thing as 1. */
   if (shdr->sh_addralign == 0)
      shdr->sh_addralign = 1;

   if (shdr->sh_link && fw->ehdr.e_shnum <= shdr->sh_link)
      return CRYPTO_INVALID_FIRMWARE;

   if (shdr->sh_type != SHT_NOBITS) {
      if (fw->len < shdr->sh_offset)
         return CRYPTO_INVALID_FIRMWARE;
      if (fw->len - shdr->sh_offset < shdr->sh_size)
         return CRYPTO_INVALID_FIRMWARE;
      if (!is_power_of_2(shdr->sh_addralign))
         return CRYPTO_INVALID_FIRMWARE;
      if ((shdr->sh_offset & (shdr->sh_addralign - 1)) != 0)
         return CRYPTO_INVALID_FIRMWARE;
      if (shdr->sh_entsize && shdr->sh_size % shdr->sh_entsize != 0)
         return CRYPTO_INVALID_FIRMWARE;
   }

   return 0;
}

int pka_fw_prepare_symtab(struct pka_fw *fw)
{
   int rc;

   rc = pka_fw_read_shdr(fw, 3, &fw->sh_symtab);
   if (rc < 0)
      return rc;
   if (fw->sh_symtab.sh_type != SHT_SYMTAB)
      return CRYPTO_INVALID_FIRMWARE;
   if (fw->sh_symtab.sh_addralign < 4)
      return CRYPTO_INVALID_FIRMWARE;
   if (fw->sh_symtab.sh_entsize != sizeof (Elf32_Sym))
      return CRYPTO_INVALID_FIRMWARE;

   rc = pka_fw_read_shdr(fw, fw->sh_symtab.sh_link, &fw->sh_strtab);
   if (rc < 0)
      return rc;
   if (fw->sh_strtab.sh_type != SHT_STRTAB)
      return CRYPTO_INVALID_FIRMWARE;
   if ((fw->base + fw->sh_strtab.sh_offset)[fw->sh_strtab.sh_size-1] != 0)
      return CRYPTO_INVALID_FIRMWARE;

   return 0;
}

int pka_fw_find_symbol(struct pka_fw *fw, const char *name, Elf32_Sym *sym)
{
   const unsigned char *symbase;
   const char *strbase;
   unsigned i, nsyms;
   int rc;

   rc = pka_fw_prepare_symtab(fw);
   if (rc < 0)
      return rc;

   symbase = fw->base + fw->sh_symtab.sh_offset;
   strbase = fw->base + fw->sh_strtab.sh_offset;
   nsyms = fw->sh_symtab.sh_size / fw->sh_symtab.sh_entsize;

   for (i = 0; i < nsyms; i++) {
      copy_sym(sym, symbase + i*fw->sh_symtab.sh_entsize);

      if (sym->st_name < fw->sh_strtab.sh_size
          && !strcmp(strbase + sym->st_name, name)) {
         return i;
      }
   }

   return CRYPTO_NOT_FOUND;
}

static int tag_parse_integer(uint32_t inval, unsigned long *out)
{
   uint32_t val = le32_to_cpu(inval);

   if ((val >> 24) != 0xf8)
      return CRYPTO_INVALID_FIRMWARE;

   *out = val & 0x00ffffff;
   return 0;
}

/*
 * Extract the MD5 hash out of the firmware tag block.  The inbase pointer
 * specifies the first hash word in the tag (i.e., tag base + 2).  The tag
 * must have 6 words after base.  The result is stored in the buffer
 * indicated by md5, which must be 16 bytes long.
 *
 * The firmware has the MD5 encoded in a rather annoying format.
 * It's stored from least to most significant byte, with 3 bytes
 * per firmware word.
 */
static int tag_parse_md5(const uint32_t *inbase, unsigned char *md5)
{
   int i;

   for (i = 0; i < 6; i++) {
      uint32_t val = le32_to_cpu(inbase[i]);

      if ((val >> 24) != 0xf8)
         return CRYPTO_INVALID_FIRMWARE;

      md5[15-3*i] = val & 0xff;
      if (i < 5) {
         md5[15-3*i-1] = (val >>  8) & 0xff;
         md5[15-3*i-2] = (val >> 16) & 0xff;
      }
   }

   return 0;
}

int pka_fw_parse_tag(const uint32_t *tag_base, unsigned long tag_buflen,
                     struct pka_fw_tag *tag_out)
{
   struct pka_fw_tag tag = {0};
   int rc;

   if (tag_buflen == 0)
      return CRYPTO_INVALID_FIRMWARE;

   rc = tag_parse_integer(tag_base[0], &tag.tag_length);
   if (rc < 0 || tag_buflen < tag.tag_length)
      return CRYPTO_INVALID_FIRMWARE;

   if (tag.tag_length < 8)
      return CRYPTO_INVALID_FIRMWARE;

   rc = tag_parse_integer(tag_base[1], &tag.timestamp);
   if (rc < 0)
      return CRYPTO_INVALID_FIRMWARE;

   rc = tag_parse_md5(&tag_base[2], tag.md5);
   if (rc < 0)
      return CRYPTO_INVALID_FIRMWARE;

   /* MD5 coverage is only available in newer firmwares. */
   if (tag.tag_length > 8) {
      rc = tag_parse_integer(tag_base[8], &tag.md5_coverage);
      if (rc < 0)
         return CRYPTO_INVALID_FIRMWARE;
   }

   *tag_out = tag;
   return 0;
}
