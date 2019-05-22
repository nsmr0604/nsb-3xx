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

int mpm_free_spacc_ctx(mpm_device *mpm, int no_ctx, int *ctxs)
{
   int x;
   unsigned long timeout;

   x = 0; 
   while (no_ctx--) {
      /* Early exit if we hit a -1 */
      if (ctxs[x] == -1) {
         return 0;
      }

      /* Release context from MPM */
      PDU_LOCK_BH(&mpm->lock);
      pdu_io_write32(mpm->regmap + MPM_CTX_CMD, MPM_CTX_CMD_REQ(ctxs[x], 0));
      timeout = 10000;
      while (--timeout && (pdu_io_read32(mpm->regmap + MPM_CTX_STAT) & 1));
      PDU_UNLOCK_BH(&mpm->lock);
      if (!timeout) {
         ELPHW_PRINT("mpm_free_spacc_ctx:: Failed to free context in MPM\n");
         return -1; 
      }

      /* Release from SPACC */
      if (spacc_ctx_release(mpm->spacc, ctxs[x++]) < 0) {
         ELPHW_PRINT("mpm_free_spacc_ctx:: Failed to free context from spacc\n");
         return -1;
      }
   }
   return 0;
}
