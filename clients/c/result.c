/*
 *  XFireDB result
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <xfiredb/xfiredb.h>

struct xfiredb_result *xfiredb_result_alloc(size_t num)
{
	void *ary;

	ary = calloc(num, sizeof(struct xfiredb_result));
	memset(ary, 0, sizeof(struct xfiredb_result) * num);

	return ary;
}

void xfiredb_result_free(void *p)
{
	xfire_free(p);
}

