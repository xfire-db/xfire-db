/*
 *  Thread sleeping
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

#ifndef __SLEEP_H__
#define __SLEEP_H__

#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <xfiredb/xfiredb.h>

#define time_after(a, b) \
	(((long)(b) - (a)) < 0L)

CDECL
extern void xfiredb_sleep(int s);
extern void xfiredb_sleep_ms(int ms);
extern void xfiredb_sleep_ns(long ns);
CDECL_END

#endif

