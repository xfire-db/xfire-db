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

/**
 * @addtogroup mem
 * @{
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <xfire/xfire.h>
#include <xfire/types.h>

/**
 * @brief Allocate a memory region.
 * @param len Number of bytes to allocate.
 * @return Allocated memory.
 */
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

/**
 * @brief Allocate a memory region.
 * @param len Number of bytes to allocate.
 * @return Allocated memory.
 * @note Allocated memory will be set to zero.
 */
void *xfire_zalloc(size_t len)
{
	void *region;

	region = xfire_alloc(len);
	memset(region, 0x0, len);

	return region;
}

/**
 * @brief Allocate an array.
 * @param num Number of elements to allocate.
 * @param size Size of each element.
 * @return Allocated memory.
 */
void *xfire_calloc(size_t num, size_t size)
{
	void *region;

	region = calloc(num, size);
	memset(region, 0x0, num*size);

	return region;
}

/**
 * @brief Reallocate a memory region.
 * @param region Region to reaallocate.
 * @param size New size of the region.
 * @return The new memory region.
 */
void *xfire_realloc(void *region, size_t size)
{
	return realloc(region, size);
}

/**
 * @brief Return allocated memory.
 * @param region Memory region to deallocate.
 */
void xfire_free(void *region)
{
	if(!region)
		return;

	free(region);
}

/** @} */

