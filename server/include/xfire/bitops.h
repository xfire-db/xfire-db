/*
 *  Binary operations
 *  Copyright (C) 2015   Michel Megens <dev@michelmegens.net>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __BITOPS_H__
#define __BITOPS_H__

#include <xfire/xfire.h>

#if defined(__x86_64) || defined(__x86_64__)
#define X86_64
#endif

CDECL
extern int test_bit(int nr, void *addr);
extern void set_bit(int nr, void *addr);
extern void clear_bit(int nr, void *addr);
extern int test_and_clear_bit(int nr, void *addr);
extern int test_and_set_bit(int nr, void *addr);
CDECL_END

#endif
