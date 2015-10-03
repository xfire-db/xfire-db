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

#include <xfire/xfire.h>
#include <xfire/types.h>
#include <xfire/os.h>

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
 * @brief Dictionary data entry.
 */
struct dict_entry {
	char *key; //!< Data key

	/**
         * @brief Data type
         */
	union {
		void *ptr; //!< Data pointer.
		u64 val_u64; //!< Unsigned 64-bit integer.
		s64 val_s64; //!< Signed 64-bit integer.
		double d; //!< Floating point integer.
	} value;

	struct dict_entry *next; //!< Next pointer.
};

/**
 * @brief Size of a single data entry.
 */
#define ENTRY_SIZE sizeof(struct dict_entry)

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
	struct dict_map map[2]; //!< Hash maps
	long rehashidx; //!< Rehashing index.

	int rehashing : 1; //!< Rehashing boolean.
	int iterators; //!< Number of safe iterators.

	xfire_mutex_t lock; //!< Mutex.
	xfire_cond_t rehash_condi; //!< Rehashing condition.
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
extern struct dict *dict_alloc(void);
extern void dict_free(struct dict *d);
extern int dict_clear(struct dict *d);

extern int dict_add(struct dict *d, const char *key, void *data, dict_type_t t);
extern int dict_delete(struct dict *d, const char *key, int free);
extern int dict_lookup(struct dict *d, const char *key,
			void *data, dict_type_t type);

extern struct dict_iterator *dict_get_safe_iterator(struct dict *d);
extern struct dict_iterator *dict_get_iterator(struct dict *d);
extern void dict_iterator_free(struct dict_iterator *it);
extern struct dict_entry *dict_iterator_next(struct dict_iterator *it);
CDECL_END
#endif

/** @} */

