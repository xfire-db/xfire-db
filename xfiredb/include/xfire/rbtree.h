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

/**
 * @addtogroup rbtree
 * @{
 */

#ifndef __RB_NODE_H__
#define __RB_NODE_H__

#include <stdio.h>

#include <xfire/xfire.h>
#include <xfire/os.h>
#include <xfire/types.h>
#include <xfire/flags.h>

/**
 * @brief Red black tree node type.
 */
typedef struct rb_node {
	struct rb_node *parent, //!< Node parent.
		      *left, //!< Node left.
		      *right, //!< Node right.
		      *next, //!< Duplicate linked list pointer.
		      *prev; //!< Linked list previous pointer.

	u64 key; //!< List key.
	atomic_flags_t flags; //!< Node flags.

	atomic_t ldepth; //!< Depth of the node.
	xfire_mutex_t lock; //!< Node lock.
	xfire_cond_t condi; //!< Node condition.
} RB_NODE;

/**
 * @brief Red black tree root.
 */
typedef struct rb_root {
	struct rb_node *tree; //!< Root node.
	atomic64_t num; //!< Number of nodes in the tree.
	xfire_spinlock_t lock; //!< Root lock.

	/**
	 * @brief Duplicate compare function pointer.
	 * @param node Node which has to be compared.
	 * @param arg Configurable argument.
	 * @return True if the node matches, false otherwise.
	 */
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
extern void rb_destroy_root(struct rb_root *root);
extern void rb_node_destroy(struct rb_node *node);
extern s32 rb_get_height(struct rb_root *root);
extern void rb_init_root(struct rb_root *root);
extern struct rb_node *rb_insert(struct rb_root *, struct rb_node*,bool);
extern struct rb_node *rb_find(struct rb_root *root, u64 key);
extern void rb_dump(struct rb_root *root, FILE *stream);
extern void rb_iterate(struct rb_root *root,
		void (*fn)(struct rb_root *,struct rb_node *, void*), void *arg);
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

/**
 * @brief Set the key of a node.
 * @param tree Node to set the key for.
 * @param key Key to set.
 */
static inline void rb_set_key(struct rb_node *tree, u64 key)
{
	if(!tree)
		return;

	tree->key = key;
}

/**
 * @brief Get the size of a red black tree.
 * @param root Root to get the size of.
 * @return The size of \p root.
 */
static inline s64 rb_get_size(struct rb_root *root)
{
	return atomic64_get(&root->num);
}

/**
 * @brief Find out if a node has duplicates.
 * @param node Node to check for duplicates.
 * @return TRUE if \p node has duplicates, FALSE otherwise.
 */
static inline bool rb_node_has_duplicates(struct rb_node *node)
{
	return node->next != NULL;
}

/**
 * @brief Get the root node from a tree.
 * @param root Red black tree root to get the root node from.
 * @return The root node.
 */
static inline struct rb_node *rb_get_root(struct rb_root *root)
{
	return root->tree;
}

/**
 * @brief Set the root noe of a tree.
 * @param root Red black tree to set the root node for.
 * @param n Node to set as root.
 * @note The current root node will be overridden.
 */
static inline void rb_set_root(struct rb_root *root, struct rb_node *n)
{
	root->tree = n;
}

/**
 * @brief Check if a node is in a tree.
 * @param node Node to check.
 * @return TRUE if the node is not in a redblack tree, FALSE otherwise.
 */
static inline bool rb_unlinked(struct rb_node *node)
{
	if(!node)
		return false;

	if(test_bit(RB_NODE_UNLINKED_FLAG, &node->flags))
		return true;

	return false;
}


CDECL_END

#endif

/** @} */

