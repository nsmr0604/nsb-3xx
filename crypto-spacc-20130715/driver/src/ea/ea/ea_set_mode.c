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

#include "elpea.h"

int ea_set_mode(ea_device *ea, int mode)
{
   int x, y;

   // if CMD1 is not present then force using CMD0 all the time
   if (ea->config.cmd1_depth == 0) {
      ea->config.rx_fifo = ea->config.tx_fifo = 0;
      ea->config.op_mode = EA_OP_MODE_FULL_DUPLEX_FAVTX;
      return 0;
   }

   if (ea->config.cmd0_depth >= ea->config.cmd1_depth) {
      x = 0;
      y = 1;
   } else {
      x = 1;
      y = 0;
   }


   switch (mode) {
      case EA_OP_MODE_FULL_DUPLEX_FAVRX:
         ea->config.rx_fifo = x;
         ea->config.tx_fifo = y;
         break;
      case EA_OP_MODE_FULL_DUPLEX_FAVTX:
         ea->config.rx_fifo = y;
         ea->config.tx_fifo = x;
         break;
      default:
         return CRYPTO_FAILED;
   }
   ea->config.op_mode = mode;

   return 0;
}
