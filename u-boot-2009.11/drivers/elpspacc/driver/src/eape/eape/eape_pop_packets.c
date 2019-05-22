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

int eape_pop_packet(eape_device *eape)
{
  uint32_t  len, retcode, sw_id, status, x, y, offset;
  unsigned long flags;
  eape_callback cb;
  void *cb_data;

  PDU_LOCK(&eape->lock, flags);

  for (offset = 0; offset < 0x40; offset += 0x20) {
     // pop [if any] data off the STAT FIFO, we can't read the STAT register until we pop so we can't even tell if there are jobs to pop until then
     while (!EAPE_STAT_STAT_FIFO_EMPTY(pdu_io_read32(eape->regmap + offset + EAPE_REG_OUT_STAT))) {
        pdu_io_write32(eape->regmap + offset + EAPE_REG_OUT_POP, 1);
        status = pdu_io_read32(eape->regmap + offset + EAPE_REG_OUT_STAT);
        len     = EAPE_STAT_LENGTH(status);
        sw_id   = EAPE_STAT_ID(status) | ((!offset)<<8); // add a 9th bit to indicate direction (0x100 == outbound, 0x000 == inbound)
        retcode = EAPE_STAT_RET_CODE(status);

        //     printk(KERN_DEBUG "SWID popped off FIFO swid==%zu, status==%zx, len==%zu retcode==%zu sttl==%zu\n", sw_id, status, len, retcode, sttl);

        // look up the sw_id
        x = eape->swid[sw_id] >> 16;    // handle
        y = eape->swid[sw_id] & 0xFFFF; // job slot inside context
        if (!((x < eape->num_ctx) && (y < MAXJOBS) && (eape->ctx[x].swid[y] == sw_id))) {
           ELPHW_PRINT("eape_pop_packets::Invalid SW_ID(%zx) returned by EAPE\n", sw_id);
           PDU_UNLOCK(&eape->lock, flags);
           return -1;
        }
        eape->swid[sw_id] = 0xFFFFFFFF;  // reset lookup so it won't match anything again

        /* Store callback information and mark job as done. */
        cb = eape->ctx[x].cb[y];
        cb_data = eape->ctx[x].cbdata[y];

        eape->ctx[x].cb[y]   = NULL;
        eape->ctx[x].done[y] = 1;
        --(eape->ctx[x].job_cnt);

        if (eape->ctx[x].dismiss && eape->ctx[x].job_cnt == 0) {
           if (eape->ctx[x].dismiss) {
              eape->ctx[x].allocated = 0;
           }
        }

        if (cb) {
           PDU_UNLOCK(&eape->lock, flags);
           cb(eape, cb_data, len, retcode, sw_id);
           PDU_LOCK(&eape->lock, flags);
        }
     }
  }
  PDU_UNLOCK(&eape->lock, flags);
  return 0;
}
