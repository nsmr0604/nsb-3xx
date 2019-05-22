#ifndef PKA_ELF_H_
#define PKA_ELF_H_

#include <libelf.h>

struct pka_elf;

struct pka_elf *pka_elf_new(int fd);
const char *pka_elf_read_strtab(struct pka_elf *elf, size_t stridx);
size_t pka_elf_get_string(struct pka_elf *elf, const char *str);
size_t pka_elf_add_string(struct pka_elf *elf, const char *str);
size_t pka_elf_add_string_checked(struct pka_elf *elf, const char *str);
int pka_elf_write(struct pka_elf *elf);
void pka_elf_free(struct pka_elf *elf);

size_t pka_elf_add_section(struct pka_elf *elf,   const char *name,
                           void *data_buf,        size_t data_len,
                           unsigned data_type,    unsigned data_align,
                           unsigned section_type, unsigned section_flags);

#endif
