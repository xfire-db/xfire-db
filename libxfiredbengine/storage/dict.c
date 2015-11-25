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

/**
 * @addtogroup dict
 * @{
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <limits.h>

#include <sys/time.h>

#include <xfiredb/xfiredb.h>
#include <xfiredb/types.h>
#include <xfiredb/dict.h>
#include <xfiredb/mem.h>
#include <xfiredb/os.h>
#include <xfiredb/error.h>

#define DICT_MINIMAL_SIZE 4

/**
 * @brief Forced resize ratio.
 * @see dict_set_resize_ratio
 *
 * When the elements/size ratio is greater or equal
 * to dict_resize_ratio, an expand will be forced upon
 * the dictionary.
 */
static int dict_resize_ratio = 5;

/**
 * @brief Indicator if dictionary's can be expanded.
 * @see dict_set_can_expand
 */
static int dict_can_expand = 1;

static void *dict_rehash_worker(void *arg);

/**
 * @brief Enable or disable the expanding of dictionary's.
 * @param x Set to TRUE if expanding is enabled, FALSE otherwise.
 */
void dict_set_can_expand(int x)
{
	dict_can_expand = !!x;
}

/**
 * @brief Set the force resize ratio.
 * @param r Ratio
 *
 * When the elements::size ratio is greater or equal to
 * \p r, an expand is forced.
 */
void dict_set_resize_ratio(int r)
{
	if(r < 0)
		return;

	dict_resize_ratio = r;
}

/**
 * @brief Check if the dictionary is rehashing.
 * @param d Dictionary to check.
 * @note struct dict::lock will be locked.
 * @return TRUE if the dictionary is rehashing, false otherwise.
 */
static inline int dict_is_rehashing(struct dict *d)
{
	int rval;

	xfiredb_mutex_lock(&d->lock);
	rval = d->rehashing != 0;
	xfiredb_mutex_unlock(&d->lock);

	return rval;
}

/**
 * @brief Get the dictionary associated with an iterator.
 * @param it Iterator to get the dictionary for.
 * @return Associated dictionary.
 */
static inline struct dict *dict_iterator_to_dict(struct dict_iterator *it)
{
	if(!it)
		return NULL;

	return it->dict;
}

/**
 * @brief Check if a dictionary has  running safe iterators.
 * @param d Dictionary to check.
 * @return TRUE if there are safe iterators running, FALSE otherwise.
 */
static inline int dict_has_iterators(struct dict *d)
{
	int rval;

	xfiredb_mutex_lock(&d->lock);
	rval = d->iterators != 0;
	xfiredb_mutex_unlock(&d->lock);

	return rval;
}

/**
 * @brief Set the value of a dictionary entry.
 * @param e Entry to set the value for.
 * @param data Value to set.
 * @param t Type of \p data.
 */
static inline void dict_set_val(struct dict_entry *e, unsigned long *data,
				dict_type_t t)
{
	switch(t) {
	case DICT_PTR:
		e->value.ptr = (void*)data;
		break;
	case DICT_U64:
		e->value.val_u64 = *((u64*)data);
		break;
	case DICT_S64:
		e->value.val_s64 = *((s64*)data);
		break;
	case DICT_FLT:
		e->value.d = *((double*)data);
		break;
	default:
		break;
	}
}

/**
 * @brief Get the number of elements in a dictionary.
 * @param d Dictionary to get the size of.
 * @return The number of elements in \p d.
 */
long dict_get_size(struct dict *d)
{
	long rv = 0L;

	xfiredb_mutex_lock(&d->lock);
	rv += d->map[PRIMARY_MAP].length;
	rv += d->map[REHASH_MAP].length;
	xfiredb_mutex_unlock(&d->lock);

	return rv;
}

/**
 * @brief Initialise a dictionary.
 * @param d Dictionary to initialise.
 */
static void dict_init(struct dict *d)
{
	d->status = DICT_STATUS_NONE;
	d->map[PRIMARY_MAP].array = xfiredb_zalloc(DICT_MINIMAL_SIZE * sizeof(size_t));
	d->map[PRIMARY_MAP].size = DICT_MINIMAL_SIZE;
	d->map[PRIMARY_MAP].sizemask = DICT_MINIMAL_SIZE - 1;
	d->map[PRIMARY_MAP].length = 0;

	xfiredb_mutex_init(&d->lock);
	xfiredb_cond_init(&d->rehash_condi);
	d->worker = xfiredb_create_thread("rehash-worker", &dict_rehash_worker, d);

	d->iterators = 0;
}

/**
 * @brief Allocate a new dictionary.
 * @return Allocated dictionary. NULL if allocation failed.
 */
struct dict *dict_alloc(void)
{
	struct dict *d;

	d = xfiredb_zalloc(sizeof(*d));

	if(!d)
		return NULL;

	dict_init(d);
	return d;
}

/**
 * @brief Free an allocated dictionary.
 * @param d Dictionary to free.
 * @note Elements that are still in the dictionary will not be
 *       free'd. Use `dict_clear' to clear out the dictionary first.
 */
void dict_free(struct dict *d)
{
	if(!d)
		return;

	if(d->map[REHASH_MAP].array)
		xfiredb_free(d->map[REHASH_MAP].array);
	if(d->map[PRIMARY_MAP].array)
		xfiredb_free(d->map[PRIMARY_MAP].array);

	xfiredb_mutex_lock(&d->lock);
	d->status = DICT_STATUS_FREE;
	xfiredb_cond_signal(&d->rehash_condi);
	xfiredb_mutex_unlock(&d->lock);

	xfiredb_thread_join(d->worker);
	xfiredb_thread_destroy(d->worker);

	xfiredb_cond_destroy(&d->rehash_condi);
	xfiredb_mutex_destroy(&d->lock);
	xfiredb_free(d);
}

/**
 * @brief Check whether a key is available or not.
 * @param d Dict to check.
 * @param key Key to check.
 * @return TRUE if the key is available, false otherwise.
 */
bool dict_key_available(struct dict *d, char *key)
{
	int found;
	union entry_data data;
	size_t size;

	found = dict_lookup(d, key, &data, &size);
	return found == -XFIREDB_OK;
}

/**
 * @brief Compare two keys.
 * @param key1 First key.
 * @param key2 Second key.
 * @return TRUE if the keys are equal, FALSE otherwise.
 */
static inline int dict_cmp_keys(const char *key1, const char *key2)
{
	if(!key1 || !key2)
		return false;

	return !strcmp(key1, key2);
}

#define MURMUR_C1 0xcc9e2d51
#define MURMUR_C2 0x1b873593
#define MURMUR_R1 15
#define MURMUR_R2 13
#define MURMUR_MIX1 5
#define MURMUR_MIX2 0xe6546b64

/**
 * @brief Hash a dictionary key.
 * @param key Key to be hashed.
 * @param seed Hashing seed.
 * @note The seed should be set to DICT_SEED in every
 *       case.
 *
 * This is an implementation if the murmur version 3 hash. See
 * https://gowalker.org/github.com/spaolacci/murmur3 for benchmark
 * results.
 */
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

/**
 * @brief Reset a dictionary map.
 * @param map Map to reset.
 *
 * All fields in the given map will be reset to zero,
 * including the data array. This means that any saved
 * data is lost.
 */
static void dict_reset(struct dict_map *map)
{
	map->array = NULL;
	map->size = 0L;
	map->length = 0L;
	map->sizemask = 0UL;
}

/**
 * @brief Get the time in miliseconds.
 * @return The time in miliseconds.
 *
 * This function utilizes gettimeofday to calculate a
 * timestamp in miliseconds.
 */
static long long dict_time_in_ms(void)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	return (((long long)tv.tv_sec) * 1000) + (tv.tv_usec / 1000);
}

#define DICT_SEED 0x8FE3C9A1

/**
 * @brief Check if a dictionary is being rehashed.
 * @param d Dictionary to check.
 * @note Does not acquire any locks.
 *
 * This is the unsafe version of dict_is_rehashing.
 */
static inline int __dict_is_rehashing(struct dict *d)
{
	return d->rehashing != 0;
}

/**
 * @brief Rehash a dictionary.
 * @param d Dictionary to rehash.
 * @param num Number of rehashing steps.
 * @return 0 if no more rehashing is required, 1 otherwise.
 * @note This function acquires struct dict::lock.
 * @see dict_rehash_worker dict_rehash_step dict_rehash_ms
 *
 * This function will perform \p num steps of rehashing. If there
 * are more than \p num*10 NULL elements found by this function, it will
 * stop and return \p 1. This means that the dictionary needs further
 * rehashing, and the dictionary worker will ensure that it happens
 * when it gets processor time.
 */
static int dict_rehash(struct dict *d, int num)
{
	u32 hash, idx;
	int visits;
	struct dict_entry *de, *next;

	xfiredb_mutex_lock(&d->lock);
	if(unlikely(!__dict_is_rehashing(d))) {
		xfiredb_mutex_unlock(&d->lock);
		return 0;
	}

	visits = num * 10;
	while(num-- && d->map[PRIMARY_MAP].length > 0L) {
		assert(d->map[PRIMARY_MAP].size > d->rehashidx);

		while(unlikely(d->map[PRIMARY_MAP].array[d->rehashidx] == NULL)) {
			d->rehashidx++;

			if(--visits == 0) {
				xfiredb_mutex_unlock(&d->lock);
				return 1;
			}
		}

		de = d->map[PRIMARY_MAP].array[d->rehashidx];
		while(likely(de)) {
			/* move an entry to the new map */
			next = de->next;
			hash = dict_hash_key(de->key, DICT_SEED);
			idx = hash & d->map[REHASH_MAP].sizemask;

			de->next = d->map[REHASH_MAP].array[idx];
			d->map[REHASH_MAP].array[idx] = de;
			d->map[REHASH_MAP].length++;
			d->map[PRIMARY_MAP].length--;

			de = next;
		}

		d->map[PRIMARY_MAP].array[d->rehashidx] = NULL;
		d->rehashidx++;

		if(d->map[PRIMARY_MAP].length == 0L) {
			xfiredb_free(d->map[PRIMARY_MAP].array);
			d->map[PRIMARY_MAP] = d->map[REHASH_MAP];
			dict_reset(&d->map[REHASH_MAP]);

			d->rehashidx = -1;
			d->rehashing = false;
			xfiredb_mutex_unlock(&d->lock);
			return 0;
		}
	}

	xfiredb_mutex_unlock(&d->lock);
	return 1;
}

/**
 * @brief Calculated the real size based on a given number.
 * @param size Size to base the new real size on.
 * @return The real size to be used by the dictionary API.
 *
 * The size of dictionary's should always be a power of two. This
 * function calculates the next power of two of any given number.
 * If, for example, \p size is set to 35, this function will return
 * 64, since 64 is the next biggest power of two.
 */
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

/**
 * @brief Expand a dictionary.
 * @param d Dictionary to expand.
 * @param size Minimum number of elements to resize to.
 *
 * Resize to atleast \p size elements. \p size will be rounded
 * up to the nearest power of two.
 */
static int dict_expand(struct dict *d, unsigned long size)
{
	struct dict_map map;
	unsigned long _size = dict_real_size(size);

	if(dict_is_rehashing(d) || d->map[PRIMARY_MAP].length > size)
		return -XFIREDB_ERR;

	if(d->map[PRIMARY_MAP].size == _size)
		return -XFIREDB_ERR;

	map.size = _size;
	map.sizemask = _size - 1;
	map.length = 0;
	map.array = xfiredb_zalloc(_size * PTR_SIZE);

	if(d->map[PRIMARY_MAP].array == NULL) {
		d->map[PRIMARY_MAP] = map;
		return -XFIREDB_OK;
	}

	d->map[REHASH_MAP] = map;
	d->rehashidx = 0;
	d->rehashing = true;

	xfiredb_cond_signal(&d->rehash_condi);
	return -XFIREDB_OK;
}

/**
 * @brief Rehash for a number of miliseconds.
 * @param d Dictionary to rehash.
 * @param ms Number of miliseconds to rehash.
 * @return Number of rehash steps done.
 */
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

/**
 * @brief The worker thread to rehash a dictionary.
 * @param arg Associated dictionary.
 *
 * Each dictionary has its own worker thread associated with it. When
 * the dictionary has to be rehashed, this worker will rehash it when
 * the CPU has no other important jobs to do.
 */
static void *dict_rehash_worker(void *arg)
{
	struct dict *d = arg;

	xfiredb_mutex_lock(&d->lock);
	while(true) {
		if(!__dict_is_rehashing(d) && d->status != DICT_STATUS_FREE)
			xfiredb_cond_wait(&d->rehash_condi, &d->lock);

		if(d->status == DICT_STATUS_FREE)
			break;

		dict_rehash_ms(d, 2);
	}

	xfiredb_mutex_unlock(&d->lock);
	xfiredb_thread_exit(NULL);
}

/**
 * @brief Do one rehashing step.
 * @param d Dictionary to rehash.
 */
static void dict_rehash_step(struct dict *d)
{
	if(!dict_has_iterators(d))
		dict_rehash(d, 1);
}

/**
 * @brief Determine if the dictionary should expand.
 * @param d Dictionary to test.
 * @return TRUE if the dictionary should expand, FALSE otherwise.
 *
 * Determines if the dictionary should expand based on the size,
 * number of elements and `dict_resize_ratio'.
 */
static inline int dict_should_expand(struct dict *d)
{
	if(d->map[PRIMARY_MAP].length >= d->map[PRIMARY_MAP].size && (dict_can_expand ||
		d->map[PRIMARY_MAP].length/d->map[PRIMARY_MAP].size < dict_resize_ratio))
		return 1;
	else
		return 0;
}

/**
 * @brief Expand only if necessary.
 * @param d Dictionary which needs potential expanding.
 * @return Error code.
 * @retval -XFIREDB_OK on success.
 * @retval -XFIREDB_ERR on error.
 */
static int dict_expand_if(struct dict *d)
{
	if(__dict_is_rehashing(d))
		return -XFIREDB_OK;

	if(d->map[PRIMARY_MAP].size == 0)
		return dict_expand(d, DICT_MINIMAL_SIZE);

	if(dict_should_expand(d)) {
		/* expand the dictionary to twice the current
		   number of elements */
		return dict_expand(d, d->map[PRIMARY_MAP].length*2);
	}

	return -XFIREDB_OK;
}

/**
 * @brief Calculate the index for a new key.
 * @param d Dictionary where \p key is to be inserted in.
 * @param key Key which has to be inserted.
 * @return Index or an error code.
 *
 * This function returns the index for new keys. If the return value is
 * 0 or greater, it is the index for the new key. Negative return value's
 * indicate an error.
 */
static int dict_calc_index(struct dict *d, const char *key)
{
	u32 hash, table, idx;
	struct dict_entry *de;

	if(dict_expand_if(d) == -1)
		return -XFIREDB_ERR;

	hash = dict_hash_key(key, DICT_SEED);
	for(table = 0; table <= 1; table++) {
		idx = hash & d->map[table].sizemask;
		de = d->map[table].array[idx];

		while(de) {
			if(!strcmp(de->key, key)) {
				/* key exists already */
				return -XFIREDB_ERR;
			}

			de = de->next;
		}

		if(!__dict_is_rehashing(d))
			break;
	}

	return idx;
}

/**
 * @brief Set a key in a dictionary entry.
 * @param e Entry to set a key for.
 * @param key Key to set.
 * @note New memory will be allocated to store the key.
 */
static inline void dict_set_key(struct dict_entry *e, const char *key)
{
	int length;
	char *_key;

	length = strlen(key);
	_key = xfiredb_zalloc(length+1);

	memcpy(_key, key, length);
	e->key = _key;
}

/**
 * @brief Free a dictionary entry.
 * @param e Entry to free.
 * @note Also the entry itself, \p e, will be deallocated.
 */
static inline void dict_free_entry(struct dict_entry *e)
{
	if(e->key)
		xfiredb_free(e->key);

	xfiredb_free(e);
}

/**
 * @brief Dictionary insert backend.
 * @param d Dictionary to add data to.
 * @param key Key to store data under.
 * @param data Data to store.
 * @param type Type of data that is being stored.
 *
 * This function works out where and how to store the given data
 * in the dictionary. If a resize (expand) is required, it will do so.
 */
static struct dict_entry *__dict_add(struct dict *d, const char *key,
					unsigned long *data, dict_type_t type)
{
	int index;
	struct dict_entry *entry;
	struct dict_map *map;

	if(dict_is_rehashing(d))
		dict_rehash_step(d);

	xfiredb_mutex_lock(&d->lock);
	index = dict_calc_index(d, key);
	if(index == -XFIREDB_ERR) {
		xfiredb_mutex_unlock(&d->lock);
		return NULL;
	}

	map = dict_is_rehashing(d) ? &d->map[REHASH_MAP] : &d->map[PRIMARY_MAP];
	entry = xfiredb_zalloc(sizeof(*entry));
	entry->next = map->array[index];
	map->array[index] = entry;
	map->length++;

	dict_set_key(entry, key);
	xfiredb_mutex_unlock(&d->lock);
	return entry;
}

/**
 * @brief Calculate the size of a data blob.
 * @param t Type of \p data.
 * @param data Data to calculate the size of.
 * @return The size of \p data.
 * @note If \p t is DICT_PTR \p data must point to a c-style string in
 *       order to calculate the size properly. If \p data points to raw
 *       data, raw_dict_add or raw_dict_update should be used.
 */
static size_t dict_get_entry_length(dict_type_t t, void *data)
{
	size_t rc;

	switch(t) {
	case DICT_PTR:
		rc = sizeof(void*);
		break;
	case DICT_U64:
	case DICT_S64:
		rc = sizeof(u64);
		break;
	case DICT_FLT:
		rc = sizeof(double);
		break;
	default:
		rc = 0;
		break;
	}

	return rc;
}

/**
 * @brief Add a new key-value pair to a dictionary.
 * @param d Dictionary to add the pair to.
 * @param key Key to be stored.
 * @param data Data to be stored.
 * @param t Type of data.
 * @return An error code. If no error occured -XFIREDB_OK will be returned.
 */
int dict_add(struct dict *d, const char *key, void *data, dict_type_t t)
{
	size_t l;

	l = dict_get_entry_length(t, data);
	return raw_dict_add(d, key, data, t, l);
}

/**
 * @brief Add a new key-value pair to a dictionary.
 * @param d Dictionary to add the pair to.
 * @param key Key to be stored.
 * @param data Data to be stored.
 * @param t Type of data.
 * @param size Length (i.e.) size of the \p data parameter.
 * @return An error code. If no error occured -XFIREDB_OK will be returned.
 */
int raw_dict_add(struct dict *d, const char *key, void *data, dict_type_t t, size_t size)
{
	struct dict_entry *e;

	e = __dict_add(d, key, data, t);
	if(!e)
		return -XFIREDB_ERR;

	dict_set_val(e, data, t);
	e->length = size;
	return -XFIREDB_OK;
}

/**
 * @brief Dictionary delete backend.
 * @param d Dictionary to delete \p key from.
 * @param key Key which needs to be deleted.
 * @return The deleted entry.
 *
 * This functions finds the given entry, deletes it and then returns
 * the deleled entry to the calling function. If the given key does
 * not exist in the dictionary, NULL is returned.
 */
static struct dict_entry *__dict_delete(struct dict *d, const char *key)
{
	u32 hash, idx;
	struct dict_entry *e, *prev_e;
	int table;

	if(dict_is_rehashing(d))
		dict_rehash_step(d);

	xfiredb_mutex_lock(&d->lock);
	if(d->map[PRIMARY_MAP].size == 0L) {
		xfiredb_mutex_unlock(&d->lock);
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
				xfiredb_mutex_unlock(&d->lock);
				return e;
			}

			prev_e = e;
			e = e->next;
		}

		if(!dict_is_rehashing(d))
			break;
	}

	xfiredb_mutex_unlock(&d->lock);
	return NULL;
}

/**
 * @brief Delete a key-value pair from a dictionary.
 * @param d Dictionary to delete from.
 * @param key Key which has to be deleted.
 * @param data Pointer to store deleted data in.
 * @param free Set to true if the stored data needs to be deallocated.
 *
 * Please note that setting \p free to true is only valid if the data type
 * is DICT_PTR.
 */
int dict_delete(struct dict *d, const char *key, union entry_data *data, int free)
{
	struct dict_entry *e;

	if(!d || !key)
		return -XFIREDB_ERR;

	e = __dict_delete(d, key);

	if(e) {
		if(free)
			xfiredb_free(e->value.ptr);
		else
			*data = e->value;
		dict_free_entry(e);
		return -XFIREDB_OK;
	}

	return -XFIREDB_ERR;
}

/**
 * @brief Dictionary lookup backend.
 * @param d Dictionary to perform a lookup on.
 * @param key Key to check for.
 *
 * Search the dictionary for \p key. After a matching hash is
 * found, the keys will be checked again using memcmp. Only if
 * memcmp confirms the right key is found the entry will be returned.
 */
static struct dict_entry *__dict_lookup(struct dict *d, const char *key)
{
	struct dict_entry *e;
	u32 hash, idx, table;

	if(dict_is_rehashing(d))
		dict_rehash_step(d);

	xfiredb_mutex_lock(&d->lock);
	if(d->map[PRIMARY_MAP].size == 0) {
		xfiredb_mutex_unlock(&d->lock);
		return NULL;
	}

	hash = dict_hash_key(key, DICT_SEED);
	for(table = 0; table <= 1; table++) {
		idx = hash & d->map[table].sizemask;
		e = d->map[table].array[idx];

		while(e) {
			if(dict_cmp_keys(key, e->key)) {
				xfiredb_mutex_unlock(&d->lock);
				return e;
			}

			e = e->next;
		}

		if(!__dict_is_rehashing(d))
			break;
	}

	xfiredb_mutex_unlock(&d->lock);
	return NULL;
}

/**
 * @brief Update a data entry.
 * @param d Dictionary containing \p key.
 * @param key Key to update.
 * @param data Data to set.
 * @param type Type of \p data.
 * @return An error code. If the data is updated or set -XFIREDB_OK is returned.
 *
 * If the given key \p key doesn't exist yet, it will be inserted.
 */
int dict_update(struct dict *d, const char *key, void *data, dict_type_t type)
{
	size_t s;

	s = dict_get_entry_length(type, data);
	return raw_dict_update(d, key, data, type, s);
}

/**
 * @brief Update a data entry.
 * @param d Dictionary containing \p key.
 * @param key Key to update.
 * @param data Data to set.
 * @param type Type of \p data.
 * @param l Length of \p data.
 * @return An error code. If the data is updated or set -XFIREDB_OK is returned.
 *
 * If the given key \p key doesn't exist yet, it will be inserted.
 */
int raw_dict_update(struct dict *d, const char *key, void *data, dict_type_t type, size_t l)
{
	struct dict_entry *e;

	e = __dict_lookup(d, key);
	if(!e)
		return dict_add(d, key, data, type);

	dict_set_val(e, data, type);
	e->length = l;
	return -XFIREDB_OK;
}

/**
 * @brief Performs a lookup on a dictionary.
 * @param d Dictionary to perform a lookup on.
 * @param key Key to look for.
 * @param data Output parameter to store the found data.
 * @param size Number of bytes stored in \p data.
 *
 * Once an entry is found, it will be stored in the memeory pointed to
 * by \p data. It is important to make sure that the memory region pointed to
 * by \p data is big enough to hold it all. If \p type is set to
 * DICT_PTR only the pointer will be stored, not the contents of the
 * pointer.
 */
int dict_lookup(struct dict *d, const char *key, union entry_data *data, size_t *size)
{
	struct dict_entry *e;

	if(!d || !key || !data)
		return -XFIREDB_ERR;

	e = __dict_lookup(d, key);
	if(!e)
		return -XFIREDB_ERR;

	*data = e->value;
	*size = e->length;
	return -XFIREDB_OK;
}

/**
 * @brief Create an iterator.
 * @param d Dict to create an iterator for.
 * @return The created iterator.
 */
static struct dict_iterator *dict_create_iterator(struct dict *d)
{
	struct dict_iterator *i;

	i = xfiredb_zalloc(sizeof(*i));
	i->dict = d;
	i->table = 0;
	i->idx = -1L;
	i->e = NULL;
	i->e_next = NULL;

	return i;
}

/**
 * @brief Create a safe iterator.
 * @param d Dict to create an iterator for.
 * @return The created iterator.
 */
struct dict_iterator *dict_get_safe_iterator(struct dict *d)
{
	struct dict_iterator *it;

	xfiredb_mutex_lock(&d->lock);
	d->iterators++;
	xfiredb_mutex_unlock(&d->lock);

	it = dict_create_iterator(d);
	it->safe = true;
	return it;
}

/**
 * @brief Create an iterator.
 * @param d Dict to create an iterator for.
 * @return The created iterator.
 */
struct dict_iterator *dict_get_iterator(struct dict *d)
{
	struct dict_iterator *it;

	it = dict_create_iterator(d);
	it->safe = false;
	return it;
}

/**
 * @brief Get the previous entry of \p e.
 * @param head List head.
 * @param e Entry to get the previous of.
 * @return The previous list entry of \p e.
 *
 * The linked list will be rolled out until e is found, then the previous of it
 * is returned.
 */
static struct dict_entry *entry_prev(struct dict_entry *head, struct dict_entry *e)
{
	struct dict_entry *carriage;

	carriage = head;

	if(head == e)
		return NULL;

	while(carriage) {
		head = carriage;
		carriage = carriage->next;
		
		if(carriage == e)
			return head;
	}

	return NULL;
}

/**
 * @brief Iterate through a dictionary.
 * @param it Dictionary iterator.
 * @return A dictionary entry.
 * @note Do not use this function and dict_iterator_next on the same iterator,
 *       especially not on non-safe iterators, as the structure of the dictionary
 *       might change, which will cause the iterator to get lost.
 */
struct dict_entry *dict_iterator_prev(struct dict_iterator *it)
{
	struct dict_map *map;
	struct dict *d;
	struct dict_entry *e;

	d = dict_iterator_to_dict(it);

	xfiredb_mutex_lock(&d->lock);
	do {
		map = &d->map[it->table];
		if(!it->e) {
			it->idx--;

			if(it->idx == -2L)
				it->idx = d->map[it->table].size - 1;

			if(it->idx <= -1L || it->idx >= map->size) {
				if(__dict_is_rehashing(d) && it->table == 0) {
					it->table++;
					it->idx = d->map[REHASH_MAP].size - 1;
					map = &d->map[REHASH_MAP];
				} else {
					break;
				}
			}

			e = map->array[it->idx];
			while(e) {
				if(e->next)
					e = e->next;
				else
					break;
			}
			it->e = e;
		} else {
			e = map->array[it->idx];
			if(e == it->e)
				it->e = NULL;
			else
				it->e = entry_prev(e, it->e);
		}

		if(it->e) {
			xfiredb_mutex_unlock(&d->lock);
			return it->e;
		}
	} while(1);

	xfiredb_mutex_unlock(&d->lock);
	return NULL;
}

/**
 * @brief Get the next entry from an iterator.
 * @param it The iterator.
 * @return The next entry.
 *
 * If there is no next entry this function will return NULL.
 */
struct dict_entry *dict_iterator_next(struct dict_iterator *it)
{
	struct dict_map *map;
	struct dict *d;

	d = dict_iterator_to_dict(it);

	xfiredb_mutex_lock(&d->lock);
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
			xfiredb_mutex_unlock(&d->lock);
			return it->e;
		}
	} while(true);

	xfiredb_mutex_unlock(&d->lock);
	return NULL;
}

/**
 * @brief Free an iterator.
 * @param it Iterator to free.
 */
void dict_iterator_free(struct dict_iterator *it)
{
	struct dict *d = it->dict;

	if(!it)
		return;

	if(it->safe) {
		xfiredb_mutex_lock(&d->lock);
		d->iterators--;
		xfiredb_mutex_unlock(&d->lock);
	}

	xfiredb_free(it);
}

/**
 * @brief Clear out a dictionary map.
 * @param map Dictionary map to clear.
 * @note Please note that no actual data is free'd.
 */
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

	xfiredb_free(map->array);
	dict_reset(map);
}

/**
 * @brief Clear out a dictionary.
 * @param d Dictionary to clear.
 * @return An error code.
 */
int dict_clear(struct dict *d)
{
	xfiredb_mutex_lock(&d->lock);
	__dict_clear(&d->map[PRIMARY_MAP]);
	__dict_clear(&d->map[REHASH_MAP]);

	d->rehashidx = -1;
	d->rehashing = false;
	xfiredb_mutex_unlock(&d->lock);

	return -XFIREDB_OK;
}

/** @} */

