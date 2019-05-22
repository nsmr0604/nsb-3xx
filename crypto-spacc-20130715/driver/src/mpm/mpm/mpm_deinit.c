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

int mpm_deinit(mpm_device *mpm)
{
   int x;

   if (mpm->pdu.freepdu)          { pdu_free(mpm->pdu.freepdu); }
   if (mpm->pdu.ondemand)         { pdu_free(mpm->pdu.ondemand); }
   if (mpm->pdu.ondemand_cb)      { pdu_free(mpm->pdu.ondemand_cb); }
   if (mpm->pdu.ondemand_cb_data) { pdu_free(mpm->pdu.ondemand_cb_data); }
   if (mpm->pdu.ondemand_mask)    { pdu_free(mpm->pdu.ondemand_mask); }
   if (mpm->key.freekey)          { pdu_free(mpm->key.freekey); }
   if (mpm->key.key_ref_cnt)      { pdu_free(mpm->key.key_ref_cnt); }
   if (mpm->chains) {
      for (x = 0; x < mpm->config.no_chains; x++) {
         if (mpm->chains[x].pdus)          { pdu_free(mpm->chains[x].pdus); }
         if (mpm->chains[x].keys)          { pdu_free(mpm->chains[x].keys); }
         if (mpm->chains[x].callbacks)     { pdu_free(mpm->chains[x].callbacks); }
         if (mpm->chains[x].callback_data) { pdu_free(mpm->chains[x].callback_data); }
      }
      pdu_free(mpm->chains);
   }

   return 0;
}

