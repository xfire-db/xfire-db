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
#define DICT_FREE -2

/**
 * Default resize ratio
 */
static int dict_resize_ratio = 5;
static int dict_can_expand = 1;

static void *dict_rehash_worker(void *arg);

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

	xfire_mutex_init(&d->lock);
	xfire_cond_init(&d->rehash_condi);
	d->worker = xfire_create_thread("rehash-worker", &dict_rehash_worker, d);

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

void dict_free(struct dict *d)
{
	if(!d)
		return;

	if(d->map[REHASH_MAP].array)
		xfire_free(d->map[REHASH_MAP].array);
	if(d->map[PRIMARY_MAP].array)
		xfire_free(d->map[PRIMARY_MAP].array);

	xfire_mutex_lock(&d->lock);
	d->rehashidx = DICT_FREE;
	xfire_cond_signal(&d->rehash_condi);
	xfire_mutex_unlock(&d->lock);

	xfire_thread_join(d->worker);
	xfire_thread_destroy(d->worker);

	xfire_cond_destroy(&d->rehash_condi);
	xfire_mutex_destroy(&d->lock);
	xfire_free(d);
}

static inline int dict_cmp_keys(const char *key1, const char *key2)
{
	if(!key1 || !key2)
		return 1;

	return !strcmp(key1, key2);
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

static inline int __dict_is_rehashing(struct dict *d)
{
	return d->rehashing != 0;
}

static int dict_rehash(struct dict *d, int num)
{
	u32 hash;
	int visits;
	struct dict_entry *de, *next;

	xfire_mutex_lock(&d->lock);
	if(!__dict_is_rehashing(d)) {
		xfire_mutex_unlock(&d->lock);
		return 0;
	}

	visits = num * 10;
	while(num-- && d->map[PRIMARY_MAP].length > 0L) {
		assert(d->map[PRIMARY_MAP].size > d->rehashidx);

		while(d->map[PRIMARY_MAP].array[d->rehashidx] == NULL) {
			d->rehashidx++;

			if(--visits == 0) {
				xfire_mutex_unlock(&d->lock);
				return 1;
			}
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
			d->map[PRIMARY_MAP] = d->map[REHASH_MAP];
			dict_reset(&d->map[REHASH_MAP]);

			d->rehashidx = -1;
			d->rehashing = false;
			xfire_mutex_unlock(&d->lock);
			return 0;
		}
	}

	xfire_mutex_unlock(&d->lock);
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

static int dict_expand(struct dict *d, unsigned long size)
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
	d->rehashing = true;
	xfire_cond_signal(&d->rehash_condi);

	return -XFIRE_OK;
}

static int dict_rehash_ms(struct dict *d, int ms)
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

static void *dict_rehash_worker(void *arg)
{
	struct dict *d = arg;

	do {
		xfire_mutex_lock(&d->lock);
		while(!dict_is_rehashing(d) && d->rehashidx != DICT_FREE)
			xfire_cond_wait(&d->rehash_condi, &d->lock);
		xfire_mutex_unlock(&d->lock);

		if(d->rehashidx == DICT_FREE)
			return NULL;

		do {
			dict_rehash_ms(d, 2);
		} while(dict_is_rehashing(d));
	} while(1);

	return NULL;
}

static void dict_rehash_step(struct dict *d)
{
	if(!dict_hash_iterators(d))
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
	if(__dict_is_rehashing(d))
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

		if(!__dict_is_rehashing(d))
			break;
	}

	return idx;
}

static inline void dict_set_key(struct dict_entry *e, const char *key)
{
	int length;
	char *_key;

	length = strlen(key);
	_key = xfire_zalloc(length+1);

	memcpy(_key, key, length);
	e->key = _key;
}

static inline void dict_free_entry(struct dict_entry *e)
{
	if(e->key)
		xfire_free(e->key);

	xfire_free(e);
}

static struct dict_entry *__dict_add(struct dict *d, const char *key,
					unsigned long *data, dict_type_t type)
{
	int index;
	struct dict_entry *entry;
	struct dict_map *map;

	if(dict_is_rehashing(d))
		dict_rehash_step(d);

	xfire_mutex_lock(&d->lock);
	index = dict_calc_index(d, key);
	if(index == -XFIRE_ERR) {
		xfire_mutex_unlock(&d->lock);
		return NULL;
	}

	map = dict_is_rehashing(d) ? &d->map[REHASH_MAP] : &d->map[PRIMARY_MAP];
	entry = xfire_zalloc(sizeof(*entry));
	entry->next = map->array[index];
	map->array[index] = entry;
	map->length++;

	dict_set_key(entry, key);
	xfire_mutex_unlock(&d->lock);
	return entry;
}

int dict_add(struct dict *d, const char *key, void *data, dict_type_t t)
{
	struct dict_entry *e;

	e = __dict_add(d, key, data, t);
	if(!e)
		return -XFIRE_ERR;

	dict_set_val(e, data, t);
	return -XFIRE_OK;
}

static struct dict_entry *__dict_delete(struct dict *d, const char *key)
{
	u32 hash, idx;
	struct dict_entry *e, *prev_e;
	int table;

	if(dict_is_rehashing(d))
		dict_rehash_step(d);

	xfire_mutex_lock(&d->lock);
	if(d->map[PRIMARY_MAP].size == 0L) {
		xfire_mutex_unlock(&d->lock);
		return NULL;
	}

	hash = dict_hash_key(key, DICT_SEED);
	
	for(table = 0; table <= 1; table++) {
		idx = hash & d->map[table].sizemask;
		e = d->map[table].array[idx];
		prev_e = NULL;

		while(e) {
			if(dict_cmp_keys(key, e->key)) {
				/* keys are confirmed and euqual, unlink the node */
				if(prev_e)
					prev_e->next = e->next;
				else
					d->map[table].array[idx] = e->next;

				d->map[table].length--;
				xfire_mutex_unlock(&d->lock);
				return e;
			}

			prev_e = e;
			e = e->next;
		}

		if(!dict_is_rehashing(d))
			break;
	}

	xfire_mutex_unlock(&d->lock);
	return NULL;
}

int dict_delete(struct dict *d, const char *key, int free)
{
	struct dict_entry *e;

	if(!d || !key)
		return -XFIRE_ERR;

	e = __dict_delete(d, key);

	if(e) {
		if(free)
			xfire_free(e->value.ptr);
		dict_free_entry(e);
		return -XFIRE_OK;
	}

	return -XFIRE_ERR;
}

static struct dict_entry *__dict_lookup(struct dict *d, const char *key)
{
	struct dict_entry *e;
	u32 hash, idx, table;

	if(dict_is_rehashing(d))
		dict_rehash_step(d);

	xfire_mutex_lock(&d->lock);
	if(d->map[PRIMARY_MAP].size == 0) {
		xfire_mutex_unlock(&d->lock);
		return NULL;
	}

	hash = dict_hash_key(key, DICT_SEED);
	for(table = 0; table <= 1; table++) {
		idx = hash & d->map[table].sizemask;
		e = d->map[table].array[idx];

		while(e) {
			if(dict_cmp_keys(key, e->key)) {
				xfire_mutex_unlock(&d->lock);
				return e;
			}

			e = e->next;
		}

		if(!dict_is_rehashing(d))
			break;
	}

	xfire_mutex_unlock(&d->lock);
	return NULL;
}

int dict_lookup(struct dict *d, const char *key, void *data, dict_type_t type)
{
	struct dict_entry *e;

	if(!d || !key || !data)
		return -XFIRE_ERR;

	e = __dict_lookup(d, key);
	if(!e)
		return -XFIRE_ERR;

	switch(type) {
	case DICT_PTR:
		*((size_t*)data) = (size_t) e->value.ptr;
		break;
	case DICT_U64:
		*((u64*)data) = e->value.val_u64;
		break;
	case DICT_S64:
		*((s64*)data) = e->value.val_s64;
		break;
	case DICT_FLT:
		*((double*)data) = e->value.d;
		break;
	default:
		return -XFIRE_ERR;
		break;
	}

	return -XFIRE_OK;
}

static struct dict_iterator *dict_create_iterator(struct dict *d)
{
	struct dict_iterator *i;

	i = xfire_zalloc(sizeof(*i));
	i->dict = d;
	i->table = 0;
	i->idx = -1L;
	i->e = NULL;
	i->e_next = NULL;

	return i;
}

struct dict_iterator *dict_get_safe_iterator(struct dict *d)
{
	struct dict_iterator *it;

	xfire_mutex_lock(&d->lock);
	d->iterators++;
	xfire_mutex_unlock(&d->lock);

	it = dict_create_iterator(d);
	it->safe = true;
	return it;
}

struct dict_iterator *dict_get_iterator(struct dict *d)
{
	struct dict_iterator *it;

	it = dict_create_iterator(d);
	it->safe = false;
	return it;
}

struct dict_entry *dict_iterator_next(struct dict_iterator *it)
{
	struct dict_map *map;
	struct dict *d;

	d = dict_iterator_to_dict(it);

	xfire_mutex_lock(&d->lock);
	do {
		if(!it->e) {
			map = &d->map[it->table];
			it->idx++;

			if(it->idx >= map->size) {
				if(__dict_is_rehashing(d) && it->table == 0) {
					it->table++;
					it->idx = 0;
					map = &d->map[REHASH_MAP];
				} else {
					break;
				}
			}

			it->e = map->array[it->idx];
		} else {
			it->e = it->e_next;
		}

		if(it->e) {
			it->e_next = it->e->next;
			xfire_mutex_unlock(&d->lock);
			return it->e;
		}
	} while(true);
	xfire_mutex_unlock(&d->lock);

	return NULL;
}

void dict_iterator_free(struct dict_iterator *it)
{
	struct dict *d = it->dict;

	if(!it)
		return;

	if(it->safe) {
		xfire_mutex_lock(&d->lock);
		d->iterators--;
		xfire_mutex_unlock(&d->lock);
	}

	xfire_free(it);
}

static void __dict_clear(struct dict_map *map)
{
	long i;
	struct dict_entry *e, *e_next;

	for(i = 0; i < map->size && map->length > 0; i++) {
		e = map->array[i];

		if(!e)
			continue;

		while(e) {
			e_next = e->next;
			dict_free_entry(e);
			map->length--;
			e = e_next;
		}
	}

	xfire_free(map->array);
	dict_reset(map);
}

int dict_clear(struct dict *d)
{
	xfire_mutex_lock(&d->lock);
	__dict_clear(&d->map[PRIMARY_MAP]);
	__dict_clear(&d->map[REHASH_MAP]);

	d->rehashidx = -1;
	d->rehashing = false;
	xfire_mutex_unlock(&d->lock);

	return -XFIRE_OK;
}

