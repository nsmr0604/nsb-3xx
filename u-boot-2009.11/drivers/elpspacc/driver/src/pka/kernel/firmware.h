#ifndef PKA_FIRMWARE_H_
#define PKA_FIRMWARE_H_

#include <linux/elf.h>

struct pka_fw_tag {
   unsigned long tag_length, timestamp, md5_coverage;
   unsigned char md5[16];
};

struct pka_fw {
   const unsigned char *base;
   unsigned long len;

   Elf32_Ehdr ehdr;
   Elf32_Shdr sh_ram;
   Elf32_Shdr sh_symtab;
   Elf32_Shdr sh_strtab;

   const u32 *ram_base, *rom_base;
   unsigned long ram_size, rom_size;
   struct pka_fw_tag ram_tag, rom_tag;
};

/*
 * The PKA timestamp epoch (2009-11-11 11:00:00Z) expressed as a UNIX
 * timestamp.
 */
#define PKA_TS_EPOCH 1257937200ul
#define PKA_TS_RESOLUTION 20

enum {
   PKA_FW_SECT_RAM = 1,
   PKA_FW_SECT_ROM,
   PKA_FW_SECT_SYM,
};

int pka_fw_init(struct pka_fw *fw, const unsigned char *data,
                                   unsigned long len);

int pka_fw_read_shdr(struct pka_fw *fw, unsigned long index, Elf32_Shdr *shdr);

int pka_fw_find_symbol(struct pka_fw *fw, const char *name, Elf32_Sym *sym);

int pka_fw_parse_tag(const uint32_t *tag_base, unsigned long tag_len,
                     struct pka_fw_tag *tag);

#endif
