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

// The following function returns the current library version value.
// XXXXVVRR
// XXXX - Build Date
// VV    - Version
// RR    - Release
#if defined(BDATE) && defined(VERSION) && defined(RELEASE) && 0
uint32_t spacc_get_version (void)
{
   return (uint32_t) (BDATE << 16) | (((uint32_t) (0x000000FF) & (uint32_t) (VERSION)) << 8) | ((0x000000FF) & (uint32_t) (RELEASE));
}
#endif
