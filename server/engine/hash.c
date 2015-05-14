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

#define XFIRE_SEED_CONSTANT 0x3BD1E995A641BC81ULL
#define XFIRE_COLLISION_CONSTANT 0x30A1F995A241BC84ULL

static void fnv_hash(const char *data, u64 seed, u64 *key)
{
	const unsigned char *_data = (unsigned char*)data;
	u64 hash = seed;

	while(*_data) {
		hash = XOR(hash, *_data);
		hash *= FNV_PRIME;
		_data++;
	}

	*key = hash;
}

static u64 xfire_calc_seed(const char *data)
{
	int idx;
	u64 tmp, seed, val, len;
	unsigned char *_data = (unsigned char *)data;

	len = strlen(data) * XFIRE_SEED_CONSTANT;
	seed = XFIRE_SEED_CONSTANT;
	val = XFIRE_COLLISION_CONSTANT;
	fnv_hash(data, FNV_BASE_HASH, &tmp);

	for(idx = 1; *_data; _data++, idx++) {
		val *= (*_data + idx) * XFIRE_COLLISION_CONSTANT;
		tmp += val;
		seed += XOR(val, len);
	}

	return seed;
}

u64 xfire_hash(const char *data, u64 *key)
{
	u64 tmp, hash;

	tmp = xfire_calc_seed(data);
	fnv_hash(data, tmp, &hash);

	if(key)
		*key = hash;

	return hash;
}

