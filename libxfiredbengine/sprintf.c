/*
 *  sprintf implementation
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

/**
 * @addtogroup stdlib
 * @{
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <xfiredb/mem.h>
#include <xfiredb/error.h>

static int xfiredb_vsprintf(char **str, const char *fmt, va_list args)
{
	int size = 0;
	va_list tmpa;

	va_copy(tmpa, args);
	size = vsnprintf(NULL, size, fmt, tmpa);
	va_end(tmpa);

	if(size < 0)
		return -XFIREDB_ERR;

	*str = xfiredb_zalloc(size + 1);
	size = vsprintf(*str, fmt, args);

	return size;
}

int xfiredb_sprintf(char **buf, const char *format, ...)
{
	int size;
	va_list args;

	va_start(args, format);
	size = xfiredb_vsprintf(buf, format, args);
	va_end(args);

	return size;
}

/** @} */

