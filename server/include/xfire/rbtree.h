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

#ifndef __RB_NODE_H__
#define __RB_NODE_H__

#include <stdio.h>

#include <xfire/xfire.h>
#include <xfire/os.h>
#include <xfire/types.h>
#include <xfire/flags.h>

typedef struct rb_node {
	struct rb_node *parent,
		      *left,
		      *right,
		      *next,
		      *prev;

	u64 key;
	atomic_flags_t flags;

	atomic_t ldepth;
	xfire_mutex_t lock;
	xfire_cond_t condi;
} RB_NODE;

typedef struct rb_root {
	struct rb_node *tree;
	atomic64_t num;
	xfire_spinlock_t lock;

	bool (*cmp)(struct rb_node *node,const void*);

} RB_ROOT;

#define RB_NODE_ACQUIRED_FLAG		0
#define RB_NODE_UNLINKED_FLAG		1
#define RB_NODE_DBLK_FLAG		2
#define RB_NODE_REMOVE_FLAG		3
#define RB_NODE_HAS_DUPLICATES_FLAG	4
#define RB_NODE_RED_FLAG		5

#define RB_RED 		true
#define RB_BLACK 	false

CDECL
extern s32 rb_get_height(struct rb_root *root);
extern void rb_init_root(struct rb_root *root);
extern struct rb_node *rb_insert(struct rb_root *, struct rb_node*,bool);
extern struct rb_node *rb_find(struct rb_root *root, u64 key);
extern void rb_dump(struct rb_root *root, FILE *stream);
extern void rb_iterate(struct rb_root *root,
		void (*fn)(struct rb_node *));
extern struct rb_node *rb_find_leftmost(struct rb_node *tree);
extern struct rb_node *rb_find_rightmost(struct rb_node *tree);

extern struct rb_node *rb_find_duplicate(struct rb_root *root, u64 key,
						const void *arg);
extern struct rb_node *rb_remove(struct rb_root *root,
				    u64 key, const void *arg);
extern void rb_init_node(struct rb_node *node);

extern void rb_put_node(struct rb_node *node);
extern struct rb_node *rb_get_node(struct rb_root *root, u64 key,
					const void *arg);
extern struct rb_node *__rb_get_node(struct rb_node *node);

static inline void rb_set_key(struct rb_node *tree, u64 key)
{
	if(!tree)
		return;

	tree->key = key;
}

static inline s64 rb_get_size(struct rb_root *root)
{
	return atomic64_get(&root->num);
}

static inline bool rb_node_has_duplicates(struct rb_node *node)
{
	return node->next != NULL;
}

static inline struct rb_node *rb_get_root(struct rb_root *root)
{
	struct rb_node *rv;

	xfire_spin_lock(&root->lock);
	rv = root->tree;
	xfire_spin_unlock(&root->lock);

	return rv;
}
static inline void rb_set_root(struct rb_root *root, struct rb_node *n)
{
	xfire_spin_lock(&root->lock);
	root->tree = n;
	xfire_spin_unlock(&root->lock);
}

static inline bool rb_unlinked(struct rb_node *node)
{
	if(!node)
		return false;

	if(test_bit(RB_NODE_UNLINKED_FLAG, &node->flags))
		return true;

	return false;
}

static inline void rb_swap_color(struct rb_node *n1, struct rb_node *n2)
{
	swap_bit(RB_NODE_RED_FLAG, &n1->flags, &n2->flags);
	swap_bit(RB_NODE_DBLK_FLAG, &n1->flags, &n2->flags);
}

static inline bool rb_red(struct rb_node *n)
{
	if(!n)
		return false;

	return test_bit(RB_NODE_RED_FLAG, &n->flags);
}

static inline bool rb_blk(struct rb_node *n)
{
	if(!n)
		return true;

	if(test_bit(RB_NODE_RED_FLAG, &n->flags) ||
			test_bit(RB_NODE_DBLK_FLAG, &n->flags))
		return false;
	
	return true;
}

static inline void rb_set_dblk(struct rb_node *n)
{
	if(!n)
		return;

	clear_bit(RB_NODE_RED_FLAG, &n->flags);
	set_bit(RB_NODE_DBLK_FLAG, &n->flags);
}

static inline void rb_set_red(struct rb_node *n)
{
	if(!n)
		return;

	clear_bit(RB_NODE_DBLK_FLAG, &n->flags);
	set_bit(RB_NODE_RED_FLAG, &n->flags);
}

static inline void rb_set_blk(struct rb_node *n)
{
	if(!n)
		return;

	clear_bit(RB_NODE_DBLK_FLAG, &n->flags);
	clear_bit(RB_NODE_RED_FLAG, &n->flags);
}

static inline bool rb_dblk(struct rb_node *n)
{
	if(!n)
		return false;

	return test_bit(RB_NODE_DBLK_FLAG, &n->flags);
}

CDECL_END

#endif
