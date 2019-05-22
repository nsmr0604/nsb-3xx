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

static void store_word(unsigned char *dst, uint32_t v)
{
#if 0
   *dst++ = (v >> 24) & 0xFF;
   *dst++ = (v >> 16) & 0xFF;
   *dst++ = (v >>  8) & 0xFF;
   *dst++ = (v >>  0) & 0xFF;
#else
   *dst++ = (v >>  0) & 0xFF;
   *dst++ = (v >>  8) & 0xFF;
   *dst++ = (v >> 16) & 0xFF;
   *dst++ = (v >> 24) & 0xFF;
#endif
}

int ea_build_sa(ea_device *ea, int handle, ea_sa *sa)
{
   unsigned char *sp;
   
   if (handle < 0 || handle > ea->num_ctx || ea->ctx[handle].allocated == 0) {
      ELPHW_PRINT("ea_build_sa::Invalid handle\n");
      return -1;
   }
   
   sp = ea->sa_ptr_virt + handle * SA_SIZE;
   
   store_word(sp+SA_CTRL, sa->ctrl);
   store_word(sp+SA_SPI,  htonl(sa->spi));
//   store_word(sp+SA_SPI,  sa->spi);
   store_word(sp+SA_SEQ_NUM+0,  sa->seqnum[1]);
   store_word(sp+SA_SEQ_NUM+4,  sa->seqnum[0]);
   store_word(sp+SA_HARD_TTL+0, sa->hard_ttl[0]);
   store_word(sp+SA_HARD_TTL+4, sa->hard_ttl[1]);
   store_word(sp+SA_SOFT_TTL+0, sa->soft_ttl[0]);
   store_word(sp+SA_SOFT_TTL+4, sa->soft_ttl[1]);
   memcpy(sp+SA_AR_MASK,     sa->armask, 32);
   memcpy(sp+SA_CIPHER_KEY,  sa->ckey,   32);
   memcpy(sp+SA_CIPHER_SALT, sa->csalt,   4);
   memcpy(sp+SA_MAC_KEY,     sa->mackey, 64);
   
#if 0
   int x, y;
   for (x = 0; x < SA_SIZE; ) {
      printk("SA %04x: ", x);
      for (y = 0; y < 4; y++) {
         printk("%02X", sp[x+y]);
      }
      printk("\n");
      x += 4;
   }
#endif   
   
   
   return 0;
}
