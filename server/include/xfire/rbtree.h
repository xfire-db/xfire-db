/*
 *  Red-black tree header
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

#ifndef __RBTREE_H__
#define __RBTREE_H__

#include <stdio.h>

#include <xfire/xfire.h>
#include <xfire/types.h>

typedef struct rbtree {
	struct rbtree *parent,
		      *left,
		      *right,
		      *root;
	u64 key;
	unsigned long flags;
} RBTREE;

typedef struct rbtree_root {
	struct rbtree *tree;
	u32 height;
	u64 num;
} RBTREE_ROOT;

#define RBTREE_IS_ROOT_FLAG 	0
#define RBTREE_READ_LOCK_FLAG 	1
#define RBTREE_WRITE_LOCK_FLAG 	2
#define RBTREE_RED_FLAG 	3

#define RB_RED 		true
#define RB_BLACK 	false

CDECL

extern struct rbtree *rbtree_insert(struct rbtree_root *, struct rbtree*);
extern struct rbtree *rbtree_find(struct rbtree_root *root, u64 key);
extern void rbtree_dump(struct rbtree_root *root, FILE *stream);

static inline void rbtree_set_key(struct rbtree *tree, u64 key)
{
	if(!tree)
		return;

	tree->key = key;
}

CDECL_END

#endif
