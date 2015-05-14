/*
 *  MEMORY management
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
#include <string.h>
#include <stdio.h>

#include <xfire/xfire.h>
#include <xfire/types.h>

void *xfire_alloc(size_t len)
{
	void *region;

	if(len)
		region = malloc(len);
	else
		return NULL;

	if(!region) {
		fputs("Memory allocation failed!\n", stderr);
		abort();
	}

	return region;
}

void *xfire_zalloc(size_t len)
{
	void *region;

	region = xfire_alloc(len);
	memset(region, 0x0, len);

	return region;
}

void *xfire_calloc(size_t num, size_t size)
{
	void *region;

	region = calloc(num, size);
	memset(region, 0x0, num*size);

	return region;
}

void *xfire_realloc(void *region, size_t size)
{
	return realloc(region, size);
}

void xfire_free(void *region)
{
	if(!region)
		return;

	free(region);
}

