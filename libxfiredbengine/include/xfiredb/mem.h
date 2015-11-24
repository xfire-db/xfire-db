/*
 *  MEM header
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

#ifndef __MEM_H__
#define __MEM_H__

#include <xfiredb/xfiredb.h>

CDECL
extern void *xfiredb_alloc(size_t len);
extern void *xfiredb_zalloc(size_t len);
extern void *xfiredb_calloc(size_t num, size_t size);
extern void xfiredb_free(void *region);
extern void *xfiredb_realloc(void *region, size_t size);
CDECL_END

#endif
