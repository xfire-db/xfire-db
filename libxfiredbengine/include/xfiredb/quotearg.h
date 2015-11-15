/*
 *  String quotation library
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

#ifndef __QUOTEARG_H__
#define __QUOTEARG_H__

#include <stdlib.h>

#include <xfiredb/xfiredb.h>
#include <xfiredb/error.h>

CDECL
extern char *xfiredb_escape_string(char *src);
extern char *xfiredb_unescape_string(char *src);
extern void xfiredb_escape_free(char *str);
CDECL_END

#endif
