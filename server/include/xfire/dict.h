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

struct dict_entry {
	char *key;

	union {
		void *val;
		u64 val_u64;
		s64 val_s64;
		double d;
	} value;
};

struct dict_map {
	struct dict_entry **map;
	long size;
	unsigned long sizemask;
	long used;
};

#define PRIMARY_MAP 0
#define REHASH_MAP  1

struct dict {
	struct dict_map map[2];
	long rehashidx;

	int rehashing : 1;
};

CDECL
extern struct dict *dict_alloc(void);
extern void dict_destroy(struct dict *d);

extern int dict_insert(char *key, void *data, unsigned long size);
extern int dict_remove(char *key);
CDECL_END

#endif

