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

#include "elpspacc.h"

// Releases a crypto context back into appropriate module's pool
int spacc_close (spacc_device * dev, int handle)
{
   int ret = CRYPTO_OK;


   if (handle < 0 || handle > SPACC_MAX_JOBS) {
      return CRYPTO_INVALID_HANDLE;
   }

   if (spacc_job_release (dev, handle)) {
      ret = CRYPTO_INVALID_HANDLE;
   }

   return ret;
}
