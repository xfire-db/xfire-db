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
};

CDECL
extern struct dict *dict_alloc(void);
extern void dict_destroy(struct dict *d);

extern int dict_insert(char *key, void *data, unsigned long size);
extern int dict_remove(char *key);

extern int dict_rehash_ms(struct dict *d, int ms);
extern void dict_rehash_step(struct dict *d);

extern int dict_expand(struct dict *d, unsigned long size);
extern int dict_rehash_ms(struct dict *d, int ms);

extern int dict_add(struct dict *d, const char *key,
			unsigned long *data, dict_type_t t);
extern int dict_delete(struct dict *d, const char *key, int free);
extern int dict_lookup(struct dict *d, const char *key,
			unsigned long *data, dict_type_t type);

static inline int dict_is_rehashing(struct dict *d)
{
	return (d->rehashing != 0);
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

