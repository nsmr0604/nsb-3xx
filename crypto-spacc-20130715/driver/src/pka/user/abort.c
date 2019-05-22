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
#include <sys/ioctl.h>

#include "pka_ioctl.h"
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

int main(int argc, char **argv)
{
   const char *dev = NULL;
   int opt, fd;

   pka_tool_init("abort", argc, argv);

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

   if (ioctl(fd, PKA_IOC_ABORT) == -1) {
      pka_tool_err(0, "PKA_IOC_ABORT");
      return EXIT_FAILURE;
   }

   elppka_device_close(fd);
   return 0;
}
