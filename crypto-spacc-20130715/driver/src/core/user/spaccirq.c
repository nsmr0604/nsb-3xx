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
 *
 * This application is used to control the /dev/spaccirq interface to reprogram
 * the IRQ handler
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include "elpspacc_irq.h"

void usage(void)
{
   printf("spaccirq: --irq_mode wd|step --epn 0x???? [--virt ??] [--wd nnnn] [--stat nnnn] [--cmd nnnn]\n");
   printf("\t--irq_mode\twd==watchdog, step==stepping IRQ\n");
   printf("\t--epn\t\tEPN of SPAcc to change (in hex, e.g. 0x0605)\n");
   printf("\t--virt\t\tVirtual SPAcc to change (default 0)\n");
   printf("\t--wd\t\tWatch dog counter (in cycles) good values are typically >15000\n");
   printf("\t--stat\t\tSTAT_CNT IRQ trigger, see the print out from loading elpspacc.ko to see the size of the STAT FIFO\n");
   printf("\t--cmd\t\tCMD_CNT IRQ trigger (for CMD0),  see the print out from loading elpspacc.ko to see the size of the CMD FIFO\n");
   exit(0);
}


int main(int argc, char **argv)
{
   elpspacc_irq_ioctl io;
   int x, y;

   memset(&io, 0, sizeof io);
   if (argc == 1) {
      usage();
   }
   for (x = 1; x < argc; x++) {
      if (!strcmp(argv[x], "--epn")) {
         io.spacc_epn = strtoul(argv[x+1], NULL, 16);
         ++x;
      } else if (!strcmp(argv[x], "--virt")) {
         io.spacc_virt = strtoul(argv[x+1], NULL, 10);
         ++x;
      } else if (!strcmp(argv[x], "--irq_mode")) {
         if (!strcmp(argv[x+1], "wd")) {
            io.irq_mode = SPACC_IRQ_MODE_WD;
         } else {
            io.irq_mode = SPACC_IRQ_MODE_STEP;
         }
         ++x;
      } else if (!strcmp(argv[x], "--wd")) {
         io.wd_value = strtoul(argv[x+1], NULL, 10);
         ++x;
      } else if (!strcmp(argv[x], "--stat")) {
         io.stat_value = strtoul(argv[x+1], NULL, 10);
         ++x;
      } else if (!strcmp(argv[x], "--cmd")) {
         io.cmd_value = strtoul(argv[x+1], NULL, 10);
         ++x;
      } else {
         usage();
      }
   }

   x = open("/dev/spaccirq", O_RDWR);
   if (!x) {
      perror("Cannot open SPAcc IRQ device");
      return -1;
   }
   y = ioctl(x, SPACC_IRQ_CMD_SET, &io);
   if (y) {
      perror("Cannot send IOCTL to SPAcc IRQ device");
   }
   close(x);
   return y;
}

