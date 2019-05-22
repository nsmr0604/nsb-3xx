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
#include "elpkep.h"

static int kep_next_job_id (void)
{
   static int id = 0;
   return (++id) & 0xFF;
}

int kep_go (kep_device * kep, pdu_ddt * src_ddt, pdu_ddt * dst_ddt, int handle)
{
   unsigned long lock_flag;
   if (kep_is_valid (kep, handle)) {
      ELPHW_PRINT ("kep_go::Invalid handle specified\n");
      return -1;
   }

   PDU_LOCK (&kep->lock, lock_flag);
   pdu_io_write32 (kep->regmap + KEP_REG_SRC_PTR, (unsigned long) src_ddt->phys);
   pdu_io_write32 (kep->regmap + KEP_REG_DST_PTR, (unsigned long) dst_ddt->phys);
   pdu_io_write32 (kep->regmap + KEP_REG_OFFSET, 0);
   pdu_io_write32 (kep->regmap + KEP_REG_LEN, src_ddt->len | (dst_ddt->len << 16));
   kep->ctx[handle].job_id = kep_next_job_id ();
   pdu_io_write32 (kep->regmap + KEP_REG_SW_CTRL, kep->ctx[handle].job_id);
   pdu_io_write32 (kep->regmap + KEP_REG_CTRL, (kep->ctx[handle].op) | (kep->ctx[handle].option << 4) | (handle << 16));
   PDU_UNLOCK (&kep->lock, lock_flag);

   return 0;
}
