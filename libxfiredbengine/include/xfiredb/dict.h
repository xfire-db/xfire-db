/*
 *  Dictionary header
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
#ifndef __DICT_H__
#define __DICT_H__

#include <xfiredb/xfiredb.h>
#include <xfiredb/error.h>
#include <xfiredb/types.h>
#include <xfiredb/os.h>

/**
 * @brief Dictionary data type.
 */
typedef enum {
	DICT_PTR, //!< Pointer type.
	DICT_U64, //!< Unsigned 64-bit integer.
	DICT_S64, //!< Signed 64-bit integer.
	DICT_FLT, //!< Floating point (double) integer.
} dict_type_t;

/**
 * Dictionary status.
 */
typedef enum {
	DICT_STATUS_NONE, //!< Dictionary normal state
	DICT_STATUS_FREE, //!< Dictionary free is waiting.
} dict_status_t;

/**
 * @brief Dictionary data entry.
 */
struct dict_entry {
	char *key; //!< Data key

	/**
         * @brief Data type
         */
	union entry_data {
		void *ptr; //!< Data pointer.
		u64 val_u64; //!< Unsigned 64-bit integer.
		s64 val_s64; //!< Signed 64-bit integer.
		double d; //!< Floating point integer.
	} value;

	size_t length; //!< Length of the data in value.
	dict_type_t type; //!< Data type indicator.
	struct dict_entry *next; //!< Next pointer.
};

/**
 * @brief Size of a single data entry.
 */
#define ENTRY_SIZE sizeof(struct dict_entry)

/**
 * @brief Dictionary map.
 */
struct dict_map {
	struct dict_entry **array; //!< Data array.
	long size; //!< Size of the data array.
	unsigned long sizemask; //!< Mask for \p size.
	long length; //!< Length of array (i.e. the number of elements).
};

#define PRIMARY_MAP 0 //!< Primary dictionary map.
#define REHASH_MAP  1 //!< Rehash map.

/**
 * @brief Dictionary data structure
 */
struct dict {
	struct dict_map map[2]; //!< Hash maps.
	dict_status_t status; //!< Dictionary status.

	long rehashidx; //!< Rehashing index.
	bool rehashing; //!< Rehashing boolean.
	int iterators; //!< Number of safe iterators.

	xfiredb_mutex_t lock; //!< Dictionary lock.
	xfiredb_cond_t rehash_condi; //!< Rehashing condition.
	struct thread *worker; //!< Rehashing worker.
};

/**
 * @brief Rehashing iterator.
 */
struct dict_iterator {
	struct dict *dict; //!< Associated dictionary.
	struct dict_entry *e, //!< Current entry.
                          *e_next; //!< Next entry.

	long idx; //!< Current index.
	int table; //!< Current rehashing table.
	int safe : 1; //!< Safe iterator table.
};

CDECL
extern long dict_get_size(struct dict *d);
extern void dict_set_can_expand(int x);
extern bool dict_key_available(struct dict *d, char *key);

extern struct dict *dict_alloc(void);
extern void dict_free(struct dict *d);
extern int dict_clear(struct dict *d);

extern int dict_add(struct dict *d, const char *key, void *data, dict_type_t t);
extern int raw_dict_add(struct dict *d, const char *key,
			void *data, dict_type_t t, size_t size);
extern int dict_delete(struct dict *d, const char *key, union entry_data *data, int free);
extern int dict_lookup(struct dict *d, const char *key, union entry_data *data, size_t *size);
extern int dict_update(struct dict *d, const char *key, void *data, dict_type_t type);
extern int raw_dict_update(struct dict *d, const char *key,
				void *data, dict_type_t type, size_t l);

extern struct dict_iterator *dict_get_safe_iterator(struct dict *d);
extern struct dict_iterator *dict_get_iterator(struct dict *d);
extern void dict_iterator_free(struct dict_iterator *it);
extern struct dict_entry *dict_iterator_next(struct dict_iterator *it);
extern struct dict_entry *dict_iterator_prev(struct dict_iterator *it);
CDECL_END
#endif

/** @} */

