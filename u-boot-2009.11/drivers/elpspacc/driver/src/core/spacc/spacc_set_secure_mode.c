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

#ifdef SECURE_MODE
void spacc_set_secure_mode (spacc_device * spacc, int src, int dst, int ddt, int global_lock)
{
   pdu_io_write32 (spacc->regmap + SPACC_REG_SECURE_CTRL,
                   (src ? SPACC_SECURE_CTRL_MS_SRC : 0) |
                   (dst ? SPACC_SECURE_CTRL_MS_DST : 0) | (ddt ? SPACC_SECURE_CTRL_MS_DDT : 0) | (global_lock ? SPACC_SECURE_CTRL_LOCK : 0));
}
#endif
