/*
 *  Hashed dictionary
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
#include <assert.h>
#include <limits.h>

#include <sys/time.h>

#include <xfire/xfire.h>
#include <xfire/types.h>
#include <xfire/dict.h>
#include <xfire/mem.h>
#include <xfire/os.h>
#include <xfire/error.h>

#define DICT_MINIMAL_SIZE 4

/**
 * Default resize ratio
 */
static int dict_resize_ratio = 5;
static int dict_can_expand = 1;

void dict_set_resize_ratio(int r)
{
	if(r < 0)
		return;

	dict_resize_ratio = r;
}

static void dict_init(struct dict *d)
{
	d->map[PRIMARY_MAP].array = xfire_zalloc(DICT_MINIMAL_SIZE * sizeof(size_t));
	d->map[PRIMARY_MAP].size = DICT_MINIMAL_SIZE;
	d->map[PRIMARY_MAP].sizemask = DICT_MINIMAL_SIZE - 1;
	d->map[PRIMARY_MAP].length = 0;

	d->iterators = 0;
}

struct dict *dict_alloc(void)
{
	struct dict *d;

	d = xfire_zalloc(sizeof(*d));

	if(!d)
		return NULL;

	dict_init(d);
	return d;
}

#define MURMUR_C1 0xcc9e2d51
#define MURMUR_C2 0x1b873593
#define MURMUR_R1 15
#define MURMUR_R2 13
#define MURMUR_MIX1 5
#define MURMUR_MIX2 0xe6546b64

static u32 dict_hash_key(const char *key, u32 seed)
{
	u32 hash, nblocks, k1, k2, len;
	const u32 *blocks;
	const char *tail;
	int i;

	len = strlen(key);
	hash = seed;
	nblocks = len / 4;
	blocks = (u32*)key;
	k2 = 0;

	for (i = 0; i < nblocks; i++) {
		k1 = blocks[i];
		k1 *= MURMUR_C1;
		k1 = (k1 << MURMUR_R1) | (k1 >> (32 - MURMUR_R1));
		k1 *= MURMUR_C2;

		hash ^= k1;
		hash = ((hash << MURMUR_R2) | (hash >> 
				(32 - MURMUR_R2))) * MURMUR_MIX1 + MURMUR_MIX2;
	}

	tail = (const char*) (key + nblocks * 4);
	switch (len & 3) {
	case 3:
		k2 ^= tail[2] << 16;
	case 2:
		k2 ^= tail[1] << 8;
	case 1:
		k2 ^= tail[0];

		k2 *= MURMUR_C1;
		k2 = (k2 << MURMUR_R2) | (k2 >> (32 - MURMUR_R1));
		k2 *= MURMUR_C2;
		hash ^= k2;
	}

	hash ^= len;
	hash ^= (hash >> 16);
	hash *= 0x85ebca6b;
	hash ^= (hash >> 13);
	hash *= 0xc2b2ae35;
	hash ^= (hash >> 16);

	return hash;
}

static void dict_reset(struct dict_map *map)
{
	map->array = NULL;
	map->size = 0L;
	map->length = 0L;
	map->sizemask = 0UL;
}

static long long dict_time_in_ms(void)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	return (((long long)tv.tv_sec) * 1000) + (tv.tv_usec / 1000);
}

#define DICT_SEED 0x8FE3C9A1

static int dict_rehash(struct dict *d, int num)
{
	u32 hash;
	int visits;
	struct dict_entry *de, *next;

	if(!dict_is_rehashing(d))
		return 0;

	visits = num * 10;
	while(num-- && d->map[PRIMARY_MAP].length) {
		assert(d->map[PRIMARY_MAP].size > d->rehashidx);

		while(d->map[PRIMARY_MAP].array[d->rehashidx] == NULL) {
			d->rehashidx++;

			if(--visits == 0)
				return 1;
		}

		de = d->map[PRIMARY_MAP].array[d->rehashidx];
		while(de) {
			/* move an entry to the new map */
			next = de->next;
			hash = dict_hash_key(de->key, DICT_SEED) & d->map[REHASH_MAP].sizemask;
			de->next = d->map[REHASH_MAP].array[hash];

			d->map[REHASH_MAP].array[hash] = de;
			d->map[REHASH_MAP].length++;
			d->map[PRIMARY_MAP].length--;

			de = next;
		}

		d->map[PRIMARY_MAP].array[d->rehashidx] = NULL;
		d->rehashidx++;

		if(d->map[PRIMARY_MAP].length == 0L) {
			xfire_free(d->map[PRIMARY_MAP].array);
			d->map[PRIMARY_MAP].array = d->map[REHASH_MAP].array;
			d->rehashidx = -1;
			d->rehashing = 0;
			dict_reset(&d->map[REHASH_MAP]);

			return 0;
		}
	}

	return 1;
}

static inline unsigned long dict_real_size(unsigned long size)
{
	if(size >= LONG_MAX)
		return LONG_MAX;

	size--;
	size |= size >> 1;
	size |= size >> 2;
	size |= size >> 4;
	size |= size >> 8;
	size |= size >> 16;
	size |= size >> 32;
	size++;

	return size;
}

int dict_expand(struct dict *d, unsigned long size)
{
	struct dict_map map;
	unsigned long _size = dict_real_size(size);

	if(dict_is_rehashing(d) || d->map[PRIMARY_MAP].length > size)
		return -XFIRE_ERR;

	if(d->map[PRIMARY_MAP].size == _size)
		return -XFIRE_ERR;

	map.size = _size;
	map.sizemask = _size - 1;
	map.length = 0;
	map.array = xfire_zalloc(_size * PTR_SIZE);

	if(d->map[PRIMARY_MAP].array == NULL) {
		d->map[PRIMARY_MAP] = map;
		return -XFIRE_OK;
	}

	d->map[REHASH_MAP] = map;
	d->rehashidx = 0;
	d->rehashing = 1;
	return -XFIRE_OK;
}

int dict_rehash_ms(struct dict *d, int ms)
{
	long long start = dict_time_in_ms();
	int num = 0;

	while(dict_rehash(d, 100)) {
		num += 100;
		if((dict_time_in_ms() - start) > ms)
			break;
	}

	return num;
}

void dict_rehash_step(struct dict *d)
{
	if(!d->iterators)
		dict_rehash(d, 1);
}

static inline int dict_should_expand(struct dict *d)
{
	if(d->map[PRIMARY_MAP].length >= d->map[PRIMARY_MAP].size && (dict_can_expand ||
		d->map[PRIMARY_MAP].length/d->map[PRIMARY_MAP].size < dict_resize_ratio))
		return 1;
	else
		return 0;
}

static int dict_expand_if(struct dict *d)
{
	if(dict_is_rehashing(d))
		return -XFIRE_OK;

	if(d->map[PRIMARY_MAP].size == 0)
		return dict_expand(d, DICT_MINIMAL_SIZE);

	if(dict_should_expand(d)) {
		/* expand the dictionary to twice the current
		   number of elements */
		return dict_expand(d, d->map[PRIMARY_MAP].length*2);
	}

	return -XFIRE_OK;
}

static int dict_calc_index(struct dict *d, const char *key)
{
	u32 hash, table, idx;
	struct dict_entry *de;

	if(dict_expand_if(d) == -1)
		return -XFIRE_ERR;

	hash = dict_hash_key(key, DICT_SEED);
	for(table = 0; table <= 1; table++) {
		idx = hash & d->map[table].sizemask;
		de = d->map[table].array[idx];

		while(de) {
			if(!strcmp(de->key, key)) {
				/* key exists already */
				return -XFIRE_ERR;
			}

			de = de->next;
		}

		if(!dict_is_rehashing(d))
			break;
	}

	return idx;
}

static inline void dict_set_key(struct dict_entry *e, const char *key)
{
}

static struct dict_entry *__dict_add(struct dict *d, const char *key,
					unsigned long *data, dict_type_t type)
{
	int index;
	struct dict_entry *entry;
	struct dict_map *map;

	if(dict_is_rehashing(d))
		dict_rehash_step(d);

	index = dict_calc_index(d, key);
	if(index == -XFIRE_ERR)
		return NULL;

	map = dict_is_rehashing(d) ? &d->map[REHASH_MAP] : &d->map[PRIMARY_MAP];
	entry = xfire_zalloc(sizeof(*entry));
	entry->next = map->array[index];
	map->array[index] = entry;
	map->length++;

	dict_set_key(entry, key);
	return entry;
}

int dict_add(struct dict *d, const char *key, unsigned long *data, dict_type_t t)
{
	struct dict_entry *e;

	e = __dict_add(d, key, data, t);
	if(!e)
		return -XFIRE_ERR;

	dict_set_val(e, data, t);
	return -XFIRE_OK;
}
