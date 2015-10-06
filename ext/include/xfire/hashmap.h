/*
 *  HASHMAP HEADER
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

#ifndef __HASHMAP__H__
#define __HASHMAP__H__

#include <stdio.h>

#include <xfire/xfire.h>
#include <xfire/os.h>
#include <xfire/types.h>
#include <xfire/flags.h>

typedef struct hashmap {
	struct map_node *tree;
	atomic64_t num;
	xfire_spinlock_t lock;

	bool (*cmp)(struct hashmap *node,const void*);
} HASHMAP;

typedef struct hm_node {
	char *key;
	u64 hash;

	struct map_node *parent,
		        *left,
		        *right,
		        *next,
		        *prev;

	atomic_flags_t flags;
	atomic_t ldepth;
	xfire_mutex_t lock;
	xfire_cond_t condi;
} HM_NODE;

#endif

