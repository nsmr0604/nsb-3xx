/*
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

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>
#include <ctype.h>
#include <getopt.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>

#include "readmemh.h"
#include "symbol.h"
#include "common.h"
#include "pkaelf.h"

static const char sopts[] = "rRo:e:S:VH";
static const struct option lopts[] = {
   { "output",  1, NULL, 'o' },
   { "entries", 1, NULL, 'e' },
   { "symbols", 1, NULL, 'S' },
   { "ram",     0, NULL, 'R' },
   { "rom",     0, NULL, 'r' },
   { "version", 0, NULL, 'V' },
   { "help",    0, NULL, 'H' },
   { 0 }
};

static void print_usage(FILE *f)
{
   fprintf(f, "usage: %s [-r|-R] [-o output] [-S symfile] [hexfile]\n", pka_tool_name());
}

static void print_block(int indent, int initial, const char *str)
{
   const char *l = str;

   while (*l) {
      const char *nl;

      nl = strchr(l, '\n');
      if (!nl) {
         printf("%*s%s\n", indent-initial, "", l);
         return;
      }

      printf("%*s%.*s\n", indent-initial, "", (int)(nl-l), l);
      initial = 0;
      l = nl+1;
   }
}

static void print_help(void)
{
   print_usage(stdout);
   puts(
"This is pka-mkimage: a tool to generate ELF binary firmware images for the\n"
"PKA Linux driver.\n");

   puts("Options:");
   for (const struct option *opt = lopts; opt->val; opt++) {
      int w = 0;

      if (opt->name) {
         w += printf("  -%c, --%s%s",  opt->val, opt->name,
                                       opt->has_arg ? "=" : "");
      } else {
         printf("  -%c", opt->val);
      }

      if (opt->has_arg) {
         if (opt->val == 'o' || opt->val == 'S' || opt->val == 'e') {
            w += printf("FILE");
         } else {
            w += printf("ARG");
         }
      }

      if (w > 18) {
         putchar('\n');
         w = 0;
      }

      switch (opt->val) {
      case 'r':
         print_block(20, w, "Generate a ROM firmware image.");
         break;
      case 'R':
         print_block(20, w, "Generate a RAM firmware image (default).");
         break;
      case 'e':
         print_block(20, w, "Read entry points from FILE.");
         break;
      case 'S':
         print_block(20, w, "Read symbol table from FILE.");
         break;
      case 'o':
         print_block(20, w, "Write output to FILE instead of standard output.");
         break;
      default:
         if (w > 0)
            putchar('\n');
      }
   }

   puts("\nFor more information, see the man page.");
}

/*
 * Validate the symbol list to ensure that all addresses are within range.
 * Returns true on success, false otherwise.
 */
static bool symbol_addrs_valid(struct symbol *symbols, unsigned limit)
{
   for (struct symbol *s = symbols; s; s = s->next) {
      if (s->address >= limit) {
         pka_tool_err(-1, "symbol %s address %#jx is out of bounds",
                          s->name, s->address);
         return false;
      }
   }

   return true;
}

/*
 * Check the tag block for sanity.  Return the number of instructions in the
 * tag on success, or 0 on failure.
 */
static unsigned check_tag(const unsigned long *code, unsigned nwords)
{
   unsigned i, taglen;

   if (nwords == 0) {
      pka_tool_err(-1, "F/W tag block is missing");
      return 0;
   }

   taglen = code[0] & 0xffffff;
   if ((code[0] & 0xff000000) != 0xf8000000 || taglen < 1 || taglen > nwords) {
      pka_tool_err(-1, "F/W tag length is invalid");
      return 0;
   }

   for (i = 1; i < taglen; i++) {
      if ((code[i] & 0xff000000) != 0xf8000000) {
         pka_tool_err(-1, "invalid instruction in F/W tag block");
         return 0;
      }
   }

   return taglen;
}

/*
 * Format the PKA instruction sequence into the binary format.  Returns the
 * number of bytes on success (4*nwords), or (size_t)-1 on failure.
 */
static size_t
serialize_code(unsigned char (**buf)[4], const unsigned long *code,
                                         unsigned nwords)
{
   size_t bytes;
   unsigned i;

   if (nwords >= SIZE_MAX/4)
      return -1;
   bytes = (size_t)nwords * 4;

   *buf = malloc(bytes);
   if (!*buf)
      return -1;

   for (i = 0; i < nwords; i++) {
      unsigned char *e = (*buf)[i];

      /* Data is little-endian */
      e[0] = code[i] & 0xff;
      e[1] = (code[i] >> 8) & 0xff;
      e[2] = (code[i] >> 16) & 0xff;
      e[3] = (code[i] >> 24) & 0xff;
   }

   return bytes;
}

/*
 * Create a RAM code section, and add the instructions to it (pass 0 for
 * nwords if this is a ROM-only image).
 *
 * Returns 0 on success, or -1 on failure.
 */
static int set_ram_text(struct pka_elf *elf, const unsigned long *code,
                                             unsigned nwords)
{
   unsigned char (*rawcode)[4];
   unsigned taglen;
   size_t rawsize;
   int rc;

   if (nwords > 0) {
      taglen = check_tag(code, nwords);
      if (taglen == 0)
         return -1;

      rawsize = serialize_code(&rawcode, code, nwords);
      if (rawsize == -1)
         return -1;

      rc = pka_elf_add_section(elf, ".ram.text", rawcode, rawsize,
                               ELF_T_WORD, 4, SHT_PROGBITS, SHF_ALLOC);
      if (rc < 0)
         return -1;
   } else {
      rc = pka_elf_add_section(elf, ".ram.null", NULL, 0,
                               ELF_T_WORD, 0, SHT_NOBITS, 0);
      if (rc < 0)
         return -1;
   }

   return 0;
}

/*
 * Create a ROM code section, and add the instructions to it (pass 0 for
 * nwords if this is a RAM-only image).
 *
 * Returns 0 on success, or -1 on failure.
 */
static int set_rom_text(struct pka_elf *elf, const unsigned long *code,
                                             unsigned nwords)
{
   unsigned char (*rawcode)[4];
   unsigned taglen;
   size_t rawsize;
   int rc;

   if (nwords > 0) {
      taglen = check_tag(code, nwords);
      if (taglen == 0)
         return -1;

      /* We only store the tag block for ROM images. */
      rawsize = serialize_code(&rawcode, code, taglen);
      if (rawsize == -1)
         return -1;

      rc = pka_elf_add_section(elf, ".rom.text", rawcode, rawsize,
                               ELF_T_WORD, 4, SHT_PROGBITS, SHF_ALLOC);
      if (rc < 0)
         return -1;
   } else {
      rc = pka_elf_add_section(elf, ".rom.null", NULL, 0,
                               ELF_T_WORD, 0, SHT_NOBITS, 0);
      if (rc < 0)
         return -1;
   }

   return 0;
}

/*
 * Format an ELF symbol table for the specified symbol list.  Returns the
 * number of symbols on success, or (size_t)-1 on failure.
 */
static size_t
serialize_symbols(struct pka_elf *elf, Elf32_Sym **buf, struct symbol *symbols)
{
   size_t i, nsyms;
   struct symbol *s;

   for (nsyms = 0, s = symbols; s; s = s->next)
      nsyms++;

   if (nsyms >= SIZE_MAX / sizeof **buf) {
      pka_tool_err(-1, "symbol table too large");
      return -1;
   }

   *buf = malloc(nsyms * sizeof **buf);
   if (!*buf) {
      pka_tool_err(0, "failed to allocate symbol buffer");
      return -1;
   }

   for (i = 0, s = symbols; s; s = s->next, i++) {
      Elf32_Sym *symbuf = &(*buf)[i];
      size_t name_id;

      assert(i < nsyms);

      name_id = pka_elf_add_string_checked(elf, s->name);
      if (name_id == -1) {
         free(*buf);
         return -1;
      }

      *symbuf = (Elf32_Sym) {
         .st_name  = name_id,
         .st_value = s->address * 4,
         .st_info  = ELF32_ST_INFO(1, STT_FUNC),
      };
   }

   assert(i == nsyms);
   return nsyms;
}

static int sym_compare(const void *a_, const void *b_)
{
   const Elf32_Sym *a = a_, *b = b_;

   if (ELF32_ST_BIND(a->st_info) < ELF32_ST_BIND(b->st_info))
      return -1;
   if (ELF32_ST_BIND(a->st_info) > ELF32_ST_BIND(b->st_info))
      return 1;
   if (a->st_name < b->st_name)
      return -1;
   if (a->st_name > b->st_name)
      return 1;
   if (a->st_value < b->st_value)
      return -1;
   if (a->st_value > b->st_value)
      return 1;

   return 0;
}

/*
 * Remove duplicates from a sorted list of symbols.  Returns the number of
 * symbols remaining.
 */
static size_t
filter_duplicate_symbols(struct pka_elf *elf, Elf32_Sym *syms, size_t nsyms)
{
   size_t lastbind = 0, lastname = -1, lastval = -1;
   size_t i, j;

   for (i = j = 0; i + j < nsyms; i++) {
      if (ELF32_ST_BIND(syms[i+j].st_info) != lastbind
          || syms[i+j].st_name != lastname)
      {
         syms[i] = syms[i+j];
         lastbind = ELF32_ST_BIND(syms[i].st_info);
         lastname = syms[i].st_name;
         lastval  = syms[i].st_value;
      } else if (syms[i+j].st_value != lastval) {
         syms[i] = syms[i+j];
         lastval = syms[i].st_value;

         if (lastbind == STB_GLOBAL) {
            pka_tool_err(-1, "warning: duplicate global symbol: %s",
                             pka_elf_read_strtab(elf, syms[i].st_name));
         }
      } else {
         j++;
         i--;
      }
   }

   return i;
}

static int set_symtab(struct pka_elf *elf, bool rom, struct symbol *symbols)
{
   Elf32_Sym *syms;
   size_t i, nsyms;

   nsyms = serialize_symbols(elf, &syms, symbols);
   if (nsyms == -1)
      return -1;

   for (i = 0; i < nsyms; i++) {
      syms[i].st_shndx = rom ? 2 : 1;
   };

   qsort(syms, nsyms, sizeof *syms, sym_compare);
   nsyms = filter_duplicate_symbols(elf, syms, nsyms);

   return pka_elf_add_section(elf, ".symtab", syms, nsyms * sizeof *syms,
                              ELF_T_SYM, 4, SHT_SYMTAB, SHF_ALLOC);
}

static int generate_image(int outfd, bool rom,
                          const unsigned long *code, unsigned nwords,
                          struct symbol *symbols)
{
   struct pka_elf *elf;
   int ret = -1;

   if (!symbol_addrs_valid(symbols, nwords))
      return -1;

   elf = pka_elf_new(outfd);
   if (!elf)
      return -1;

   if (set_ram_text(elf, code, rom ? 0 : nwords) == -1)
      goto out;

   if (set_rom_text(elf, code, rom ? nwords : 0) == -1)
      goto out;

   if (symbols && set_symtab(elf, rom, symbols) == -1)
      goto out;

   ret = pka_elf_write(elf);
out:
   pka_elf_free(elf);
   return ret;
}

/*
 * Parse a symbol input file.  If entfile is true, the file is interpreted
 * as a .ent file which requires some mangling of the symbol names.
 */
struct symbol *read_symbols(FILE *f, bool entfile)
{
   struct symbol *symbols;

   symbols = pka_parse_fw_sym(f);
   if (!symbols)
      return NULL;

   if (entfile) {
      struct symbol **p = &symbols;

      /* Rewrite all the symbols in uppercase with an ENTRY_ prefix. */
      for (struct symbol *s = *p; s; p = &s->next, s = *p) {
         size_t n = strlen(s->name);
         struct symbol *tmp;

         tmp = malloc(sizeof *tmp + sizeof "ENTRY_" + n);
         if (!tmp) {
            pka_free_symbols(symbols);
            return NULL;
         }

         for (size_t i = 0; i < n; i++) {
            s->name[i] = toupper(s->name[i]);
         }

         memcpy(tmp, s, sizeof *tmp);
         sprintf(tmp->name, "ENTRY_%s", s->name);

         *p = tmp;
         free(s);
         s = tmp;
      }
   }

   return symbols;
}

int main(int argc, char **argv)
{
   char *outfile = NULL, *symfile = NULL;
   FILE *fsym = NULL, *fhex = stdin;
   int outfd = STDOUT_FILENO;

   unsigned long code[PKA_MAX_FW_WORDS];
   struct symbol *symbols = NULL;
   int rc, size, opt;
   bool rom = false;
   bool sym_is_ent;

   pka_tool_init("mkimage", argc, argv);

   while ((opt = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
      switch (opt) {
      case 'o':
         outfile = optarg;
         break;
      case 'r':
         rom = true;
         break;
      case 'R':
         rom = false;
         break;
      case 'e':
         sym_is_ent = true;
         symfile = optarg;
         break;
      case 'S':
         sym_is_ent = false;
         symfile = optarg;
         break;
      case 'V':
         pka_tool_version();
         return EXIT_SUCCESS;
      case 'H':
         print_help();
         return EXIT_SUCCESS;
      default:
         print_usage(stderr);
         return EXIT_FAILURE;
      }
   }

   /* Read and parse input files */
   if (argv[optind] && (fhex = fopen(argv[optind], "r")) == NULL) {
      pka_tool_err(0, "failed to open %s", argv[optind]);
      return EXIT_FAILURE;
   }

   if (symfile && (fsym = fopen(symfile, "r")) == NULL) {
      pka_tool_err(0, "failed to open %s", symfile);
      return EXIT_FAILURE;
   } else if (!symfile) {
      pka_tool_err(-1, "no symbol or entry file specified");
      pka_tool_err(-1, "generated image will not have symbols");
      /* non-fatal */
   } else {
      symbols = read_symbols(fsym, sym_is_ent);
      if (!symbols)
         return EXIT_FAILURE;
   }

   size = pka_parse_fw_hex(fhex, code);
   if (size < 0)
      return EXIT_FAILURE;

   /* Write output file */
   if (outfile && (outfd = creat(outfile, 0666)) == -1) {
      pka_tool_err(0, "failed to open %s", outfile);
      return EXIT_FAILURE;
   }

   rc = generate_image(outfd, rom, code, size, symbols);
   if (rc != 0)
      return EXIT_FAILURE;

   return 0;
}
