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
#include "diag/elpcurve.h"
#include "diag/SECP_160.inc"
#include "diag/SECP_521.inc"

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

static int dumb_pver521(int fd)
{
   const EC_CURVE_DATA *curve = &SEC521R1;
   int rc;

   rc = elppka_run(fd, "pver", curve->size,
                        "%A2", curve->x,
                        "%B2", curve->y,
                        "%A6", curve->a,
                        "%A7", curve->b,
                        "%D0", curve->m,
                        "%D1", curve->mp,
                        "%D3", curve->r,
                        (char *)NULL);
   if (rc == -1) {
      pka_tool_err(0, "pver");
      return -1;
   } else if (rc > 0) {
      pka_tool_err(-1, "pver: device returned %d", rc);
      return -1;
   }

   printf("F1 flag: %d\n", elppka_test_flag(fd, NULL, "F1"));

   rc = elppka_test_flag(fd, NULL, "Z");
   if (rc == -1) {
      pka_tool_err(0, "pver");
      return -1;
   } else if (rc == 0) {
      pka_tool_err(-1, "pver: base point not on curve?");
      return -1;
   }

   return 0;
}

int main(int argc, char **argv)
{
   const char *dev = NULL;
   int opt, fd;

   pka_tool_init("simple521", argc, argv);

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

   dumb_pver521(fd);

   elppka_device_close(fd);
   return 0;
}
