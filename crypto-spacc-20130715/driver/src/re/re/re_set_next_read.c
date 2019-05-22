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


#include "elpre.h"
#include "elprehw.h"


/* 
 *  Sets the next read parameters. Much the same as re_set_next_write
 */
int re_set_next_read (re_device * re, int handle, unsigned char * iv, uint32_t ivlen, unsigned char * key, uint32_t keylen, unsigned char * mackey, uint32_t mackeylen,
                      unsigned char * params, uint32_t paramlen, unsigned char * sequence_number, uint32_t seqlen)
{
   int err;
   err = re_set_next_read_iv (re, handle, iv, ivlen);
   if (err != 0) {
      return err;
   }
   err = re_set_next_read_key (re, handle, key, keylen);
   if (err != 0) {
      return err;
   }
   err = re_set_next_read_mackey (re, handle, mackey, mackeylen);
   if (err != 0) {
      return err;
   }
   err = re_set_next_read_params (re, handle, params, paramlen);
   if (err != 0) {
      return err;
   }
   err = re_set_next_read_sequence_number (re, handle, sequence_number, seqlen);
   return err;
}
