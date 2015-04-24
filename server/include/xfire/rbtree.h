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
#include <xfire/os.h>
#include <xfire/types.h>
#include <xfire/flags.h>

typedef struct rbtree {
	struct rbtree *parent,
		      *left,
		      *right;

	struct list_head {
		struct list_head *next,
				 *prev;
	} duplicates;
	u64 key;
	atomic_flags_t flags;

	atomic_t ldepth;
	xfire_mutex_t lock;
	xfire_cond_t condi;
} RBTREE;

typedef struct rbtree_root {
	struct rbtree *tree;
	u32 height;
	u64 num;
	xfire_spinlock_t lock;

	atomic_flags_t flags;

	bool (*iterate)(struct rbtree *node,void*);

} RBTREE_ROOT;

#define RBROOT_BUSY_FLAG 0

#define RBTREE_HAS_DUPLICATES_FLAG 0
#define RBTREE_ACQUIRED_FLAG	   1
#define RBTREE_LOCKED_FLAG	   2
#define RBTREE_UNLINKED_FLAG	   3
#define RBTREE_DBLK_FLAG	   4
#define RBTREE_REMOVE_FLAG	   5
#define RBTREE_RED_FLAG		   6

#define RB_RED 		true
#define RB_BLACK 	false

CDECL
extern void rbtree_init_root(struct rbtree_root *root);
extern struct rbtree *rbtree_insert_duplicate(struct rbtree_root *root,
					      struct rbtree      *node);
extern struct rbtree *rbtree_insert(struct rbtree_root *, struct rbtree*);
extern struct rbtree *rbtree_find(struct rbtree_root *root, u64 key);
extern void rbtree_dump(struct rbtree_root *root, FILE *stream);
extern void rbtree_iterate(struct rbtree_root *root,
		void (*fn)(struct rbtree *));
extern struct rbtree *rbtree_find_leftmost(struct rbtree *tree);
extern struct rbtree *rbtree_find_rightmost(struct rbtree *tree);

extern struct rbtree *rbtree_find_duplicate(struct rbtree_root *root, u64 key,
		bool (*cmp)(struct rbtree*,void*), void *arg);
extern struct rbtree *rbtree_remove(struct rbtree_root *root,
				    u64 key,void *arg);
extern void rbtree_init_node(struct rbtree *node);

extern void rbtree_put_node(struct rbtree *node);
extern struct rbtree *rbtree_get_node(struct rbtree_root *root, u64 key,
		bool (*cmp)(struct rbtree*, void*), void *arg);
extern struct rbtree *__rbtree_get_node(struct rbtree *node);

static inline bool rb_ignore(struct rbtree *node)
{
	if(node)
		return false;

	return false;
}

static inline void rbtree_set_key(struct rbtree *tree, u64 key)
{
	if(!tree)
		return;

	tree->key = key;
}

static inline struct rbtree *rbtree_get_root(struct rbtree_root *root)
{
	struct rbtree *rv;

	xfire_spin_lock(&root->lock);
	rv = root->tree;
	xfire_spin_unlock(&root->lock);

	return rv;
}
static inline void rbtree_set_root(struct rbtree_root *root, struct rbtree *n)
{
	xfire_spin_lock(&root->lock);
	root->tree = n;
	xfire_spin_unlock(&root->lock);
}

static inline bool rb_unlinked(struct rbtree_root *root, struct rbtree *node)
{
	struct rbtree *rn = rbtree_get_root(root);

	if(!node)
		return false;

	if(rn == node)
		return false;

	if(test_bit(RBTREE_UNLINKED_FLAG, &node->flags))
		return true;

	return false;
}

static inline bool rb_red(struct rbtree *n)
{
	if(!n)
		return false;

	return test_bit(RBTREE_RED_FLAG, &n->flags);
}

static inline bool rb_blk(struct rbtree *n)
{
	if(!n)
		return true;

	if(test_bit(RBTREE_RED_FLAG, &n->flags) ||
			test_bit(RBTREE_DBLK_FLAG, &n->flags))
		return false;
	
	return true;
}

static inline void rb_set_dblk(struct rbtree *n)
{
	if(!n)
		return;

	clear_bit(RBTREE_RED_FLAG, &n->flags);
	set_bit(RBTREE_DBLK_FLAG, &n->flags);
}

static inline void rb_set_red(struct rbtree *n)
{
	if(!n)
		return;

	clear_bit(RBTREE_DBLK_FLAG, &n->flags);
	set_bit(RBTREE_RED_FLAG, &n->flags);
}

static inline void rb_set_blk(struct rbtree *n)
{
	if(!n)
		return;

	clear_bit(RBTREE_DBLK_FLAG, &n->flags);
	clear_bit(RBTREE_RED_FLAG, &n->flags);
}

static inline bool rb_dblk(struct rbtree *n)
{
	if(!n)
		return false;

	return test_bit(RBTREE_DBLK_FLAG, &n->flags);
}

CDECL_END

#endif
