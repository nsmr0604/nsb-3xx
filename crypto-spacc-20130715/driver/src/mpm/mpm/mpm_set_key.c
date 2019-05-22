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

#include "elpmpm.h"
int mpm_set_key(mpm_device *mpm, int key_idx, int ckey_sz, int hkey_sz, int no_cache, int inv_ckey, unsigned char *ckey, unsigned char *hkey)
{
   uint32_t ctrl;

   if (key_idx < 0 || key_idx > mpm->config.no_keys) {
      ELPHW_PRINT("mpm_set_key::Invalid key idx\n");
      return -1;
   }

   ctrl = KEY_BUF_CTRL_CKEY_SZ(ckey_sz) | 
          KEY_BUF_CTRL_HKEY_SZ(hkey_sz) |
          (no_cache ? KEY_BUF_CTRL_NOCACHE : 0) | 
          (inv_ckey ? KEY_BUF_CTRL_INV_CKEY: 0);

   mpm->key.keys[key_idx][0] = ctrl;

   if (ckey) {
      memcpy(&mpm->key.keys[key_idx][0x40/4], ckey, ckey_sz);
   }
   if (hkey) {
      memcpy(&mpm->key.keys[key_idx][0x80/4], hkey, hkey_sz);
   }
   //pdu_sync_single_for_device(mpm->key.keys_phys + (key_idx * sizeof(keybuf)), sizeof(keybuf));
   
#if 0
{
   int x;
   for (x = 0; x < 0x100/4; x++) {
       printk(KERN_DEBUG "key[0x%03x] = %08zx\n", x*4, mpm->key.keys[key_idx][x]);
   }
}
#endif      

   return 0;
}

