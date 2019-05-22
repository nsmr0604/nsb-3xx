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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <getopt.h>

#include "pkadev.h"
#include "common.h"

static const char sopts[] = "d:VH";
static const struct option lopts[] = {
   { "device",  1, NULL, 'd' },
   { "version", 0, NULL, 'V' },
   { "help",    0, NULL, 'H' },
   { 0 }
};

static void print_usage(FILE *f)
{
   fprintf(f, "usage: %s [-d device]\n", pka_tool_name());
}

static void dump_param(unsigned size, const unsigned char *buf, int indent)
{
   unsigned i;

   for (i = 0; i < size; i++) {
      if (i % 16 == 0)
         printf("%*s%.4x:", indent, "", i);
      if (i % 2 == 0)
         putchar(' ');

      printf("%.2x", (unsigned)buf[i]);

      if ((i+1) % 16 == 0)
         putchar('\n');
   }

   if (i % 16)
      putchar('\n');
}

static int dumb_modadd(int fd)
{
   unsigned char u[20], v[20], m[20];
   int rc, i;

   for (i = 0; i < 20; i++) {
      u[i] = i;
      v[i] = i + (256-20);
      m[i] = 255;
   }

   puts("result = u + v mod m");
   puts("u");
   dump_param(sizeof u, u, 3);
   puts("v");
   dump_param(sizeof v, v, 3);
   puts("m");
   dump_param(sizeof m, m, 3);

   rc = elppka_run(fd, "modadd", sizeof m,
                                 "%A0",  u,
                                 "%B0",  v,
                                 "%D0",  m,
                                 "=%A0", u,
                                 (char *)NULL); 
   if (rc == -1) {
      pka_tool_err(0, "modadd");
      return -1;
   } else if (rc > 0) {
      pka_tool_err(-1, "modadd: device returned %d", rc);
      return -1;
   }

   puts("result");
   dump_param(sizeof u, u, 3);
   return 0;
}

int main(int argc, char **argv)
{
   const char *dev = NULL;
   int opt, fd;

   pka_tool_init("dumbtest", argc, argv);

   while ((opt = getopt_long(argc, argv, sopts, lopts, NULL)) != -1) {
      switch (opt) {
      case 'd':
         dev = optarg;
         break;
      case 'V':
         pka_tool_version();
         return EXIT_SUCCESS;
      case 'H':
         print_usage(stdout);
         return EXIT_SUCCESS;
      default:
         print_usage(stderr);
         return EXIT_FAILURE;
      }
   }

   fd = elppka_device_open(dev);
   if (fd == -1) {
      if (dev)
         pka_tool_err(0, "%s", dev);
      else
         pka_tool_err(0, NULL);
      return EXIT_FAILURE;
   }

   dumb_modadd(fd);

   elppka_device_close(fd);
   return 0;
}
