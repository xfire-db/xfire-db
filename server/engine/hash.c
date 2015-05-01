/*
 *  XFIRE HASHING
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

#include <xfire/xfire.h>
#include <xfire/types.h>
#include <xfire/hash.h>

#define FNV_BASE_HASH 14695981039346656037ULL
#define FNV_PRIME 1099511628211ULL
#define XFIRE_CONSTANT 0x5BD1e995

static void fnv_hash(const char *data, u64 seed, u64 *key)
{
	const unsigned char *_data = (unsigned char*)data;
	u64 hash = seed;

	while(*_data) {
		hash ^= *_data;
		hash *= FNV_PRIME;
		_data++;
	}

	*key = hash;
}

static void xfire_calc_seed(const char *data, u64 *seed)
{
	int len;
	u64 tmp;

	len = strlen(data);
	fnv_hash(data, FNV_BASE_HASH, &tmp);
	*seed = tmp ^ (len * XFIRE_CONSTANT);
}

u64 xfire_hash(const char *data, u64 *key)
{
	u64 tmp, hash;

	xfire_calc_seed(data, &tmp);
	fnv_hash(data, tmp, &hash);

	if(key)
		*key = hash;

	return hash;
}

