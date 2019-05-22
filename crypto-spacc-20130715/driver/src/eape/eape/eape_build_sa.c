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

#include "eape.h"

static void store32(uint32_t v, unsigned char *dst)
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


int eape_build_sa(eape_device *eape, int handle, eape_sa *sa)
{
   unsigned char *buf;

   if (handle < 0 || handle > eape->num_ctx || eape->ctx[handle].allocated == 0) {
      ELPHW_PRINT("eape_build_sa::Invalid handle\n");
      return -1;
   }

   buf = eape->sa_ptr_virt + handle * SA_SIZE;

   memset(buf, 0, SA_SIZE);
   store32(sa->seqnum[1], buf+EAPE_SA_SEQNUM);
   store32(sa->seqnum[0], buf+EAPE_SA_SEQNUM+4);
   memcpy(buf+EAPE_SA_CIPH_KEY,  sa->ckey,      32);
   memcpy(buf+EAPE_SA_SALT,      sa->csalt,      4);
   memcpy(buf+EAPE_SA_AUTH_KEY1, sa->mackey,    20);
   memcpy(buf+EAPE_SA_AUTH_KEY2, sa->mackey+20, 12);
   store32(sa->spi, buf+EAPE_SA_REMOTE_SPI);
   store32(sa->hard_ttl[1], buf+EAPE_SA_HARD_TTL_HI);
   store32(sa->hard_ttl[0], buf+EAPE_SA_SOFT_TTL_LO);
   store32(sa->soft_ttl[1], buf+EAPE_SA_HARD_TTL_HI);
   store32(sa->soft_ttl[0], buf+EAPE_SA_SOFT_TTL_LO);
   buf[EAPE_SA_ALG]   = sa->alg;
   sa->flags |= EAPE_SA_FLAGS_ACTIVE;
   buf[EAPE_SA_FLAGS]   = (sa->flags>>8)&0xFF;
   buf[EAPE_SA_FLAGS+1] = sa->flags&0xFF;

/* All of the bytes are read in big endian fashion, so this code will swap all of the words around.
   The above store32 should store in a way such that their final orientation is big endian.  In this case
   we store as little endian and then swap everything

   If you're on a natively big endian platform you'll want to flip the #if blocks around.
 */

#if 0
{
   int x;
   uint32_t *p = buf;
//   printk("SA ==\n");
   for (x = 0; x < 32; x++) {
      p[x] = htonl(p[x]);
//      printk("SA[0x%02x] == 0x%08zx\n", x*4, p[x]);
   }
}
#endif

   return 0;
}

