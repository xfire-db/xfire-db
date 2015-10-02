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

#ifndef __DICT_H__
#define __DICT_H__

#include <xfire/xfire.h>
#include <xfire/types.h>
#include <xfire/os.h>

typedef enum {
	DICT_PTR,
	DICT_U64,
	DICT_S64,
	DICT_FLT,
} dict_type_t;

struct dict_entry {
	char *key;

	union {
		void *ptr;
		u64 val_u64;
		s64 val_s64;
		double d;
	} value;

	struct dict_entry *next;
};

#define ENTRY_SIZE sizeof(struct dict_entry)

struct dict_map {
	struct dict_entry **array;
	long size;
	unsigned long sizemask;
	long length;
};

#define PRIMARY_MAP 0
#define REHASH_MAP  1

struct dict {
	struct dict_map map[2];
	long rehashidx;

	int rehashing : 1;
	int iterators;

	xfire_mutex_t lock;
	xfire_cond_t rehash_condi;
	struct thread *worker;
};

struct dict_iterator {
	struct dict *dict;
	struct dict_entry *e, *e_next;

	long idx;
	int table;
	int safe : 1;
};

CDECL
extern struct dict *dict_alloc(void);
extern void dict_free(struct dict *d);

extern int dict_add(struct dict *d, const char *key, void *data, dict_type_t t);
extern int dict_delete(struct dict *d, const char *key, int free);
extern int dict_lookup(struct dict *d, const char *key,
			void *data, dict_type_t type);

extern struct dict_iterator *dict_get_safe_iterator(struct dict *d);
extern struct dict_iterator *dict_get_iterator(struct dict *d);
extern void dict_iterator_free(struct dict_iterator *it);
extern struct dict_entry *dict_iterator_next(struct dict_iterator *it);

static inline int dict_is_rehashing(struct dict *d)
{
	int rval;

	xfire_mutex_lock(&d->lock);
	rval = d->rehashing != 0;
	xfire_mutex_unlock(&d->lock);

	return rval;
}

static inline struct dict *dict_iterator_to_dict(struct dict_iterator *it)
{
	if(!it)
		return NULL;

	return it->dict;
}

static inline int dict_hash_iterators(struct dict *d)
{
	int rval;

	xfire_mutex_lock(&d->lock);
	rval = d->iterators != 0;
	xfire_mutex_unlock(&d->lock);

	return rval;
}

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
CDECL_END

#endif

