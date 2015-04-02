/*
 *  Red-black tree library
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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include <xfire/xfire.h>
#include <xfire/types.h>
#include <xfire/bitops.h>
#include <xfire/rbtree.h>

static u32 rbtree_insert_balance(struct rbtree_root *root, struct rbtree *node);

static struct rbtree *__rbtree_insert(struct rbtree_root *root,
				      struct rbtree *node)
{
	struct rbtree *tree = root->tree;

	root->num += 1ULL;
	if(!tree) {
		root->tree = node;
		set_bit(RBTREE_IS_ROOT_FLAG, &node->flags);
		clear_bit(RBTREE_RED_FLAG, &node->flags);
		return root->tree;
	}

	for(;;) {
		if(node->key <= tree->key) {
			if(tree->left == NULL) {
				tree->left = node;
				node->parent = tree;
				break;
			}

			tree = tree->left;
		} else {
			if(tree->right == NULL) {
				tree->right = node;
				node->parent = tree;
				break;
			}

			tree = tree->right;
		}
	}

	return node;
}

static struct rbtree *__rbtree_search(struct rbtree *tree, u64 key)
{
	if(!tree)
		return NULL;

	if(tree->key == key)
		return tree;

	if(key < tree->key)
		return __rbtree_search(tree->left, key);
	else
		return __rbtree_search(tree->right, key);
}

struct rbtree *rbtree_find(struct rbtree_root *root, u64 key)
{
	return __rbtree_search(root->tree, key);
}

struct rbtree *rbtree_insert(struct rbtree_root *root, struct rbtree *node)
{
	struct rbtree *entry;

	node->left = NULL;
	node->right = NULL;
	node->parent = NULL;

	entry = rbtree_find(root, node->key);
	if(entry)
		return entry;

	set_bit(RBTREE_RED_FLAG, &node->flags);
	__rbtree_insert(root, node);
	root->height = rbtree_insert_balance(root, node);
	return node;
}

static void rbtree_rotate_right(struct rbtree_root *root, struct rbtree *node)
{
	struct rbtree *left = node->left,
		      *parent = node->parent;

	node->left = left->right;
	left->parent = parent;
	node->parent = left;
	left->right = node;

	if(node->left)
		node->left->parent = node;

	if(parent) {
		if(node == parent->right)
			parent->right = left;
		else
			parent->left = left;
	} else {
		clear_bit(RBTREE_IS_ROOT_FLAG, &root->tree->flags);
		set_bit(RBTREE_IS_ROOT_FLAG, &left->flags);
		root->tree = left;
	}
}

static void rbtree_rotate_left(struct rbtree_root *root, struct rbtree *node)
{
	struct rbtree *right = node->right,
		      *parent = node->parent;

	node->right = right->left;
	right->parent = parent;
	right->left = node;
	node->parent = right;

	if(node->right)
		node->right->parent = node;

	if(parent) {
		if(node == parent->left)
			parent->left = right;
		else
			parent->right = right;
	} else {
		clear_bit(RBTREE_IS_ROOT_FLAG, &root->tree->flags);
		set_bit(RBTREE_IS_ROOT_FLAG, &right->flags);
		root->tree = right;
	}
}

static void rbtree_rotate_swap_parent(struct rbtree_root *root,
				      struct rbtree *current)
{
	struct rbtree *parent = current->parent;

	if(parent->right == current)
		rbtree_rotate_left(root, parent);
	else
		rbtree_rotate_right(root, parent);

	swap_bit(RBTREE_RED_FLAG, &parent->flags, &current->flags);
}

static inline struct rbtree *rbtree_grandparent(struct rbtree *node)
{
	struct rbtree *parent = node->parent;

	if(parent)
		return parent->parent;
	else
		return NULL;
}

static inline struct rbtree *rbtree_sibling(struct rbtree *node)
{
	struct rbtree *parent = node->parent;

	if(parent) {
		if(node == parent->left)
			return parent->right;
		else
			return parent->left;
	}

	return NULL;
}

static inline bool rbtree_node_is_right(struct rbtree *node)
{
	if(node->parent && node->parent->right == node)
		return true;
	else
		return false;
}

#define should_rotate_left(__n) rbtree_node_is_right(__n) && \
				test_bit(RBTREE_RED_FLAG, &__n->left->flags)
#define should_rotate_right(__n) !rbtree_node_is_right(__n) && \
				!test_bit(RBTREE_RED_FLAG, &__n->left->flags)
static u32 rbtree_insert_balance(struct rbtree_root *root, struct rbtree *node)
{
	struct rbtree *sibling;
	double height;

	if(node == root->tree)
		return root->height;

	node = node->parent;
	while(node != root->tree && test_bit(RBTREE_RED_FLAG, &node->flags)) {
		sibling = rbtree_sibling(node);
		if(sibling) {
			if(test_and_clear_bit(RBTREE_RED_FLAG, 
						&sibling->flags)) {
				clear_bit(RBTREE_RED_FLAG, &node->flags);
				set_bit(RBTREE_RED_FLAG, &node->parent->flags);
				node = rbtree_grandparent(node);
				
				if(!node)
					break;
				continue;
			/*
			 * Rotate in the direction of the relation between
			 * node::parent and node
			 */
			} else if(should_rotate_left(node)) {
				rbtree_rotate_left(root, node);
				node = node->parent;
			} else if(should_rotate_right(node)) {
				rbtree_rotate_right(root, node);
				node = node->parent;
			}

			rbtree_rotate_swap_parent(root, node);
		} else {
			rbtree_rotate_swap_parent(root, node);
		}
	}

	clear_bit(RBTREE_RED_FLAG, &root->tree->flags);
	height = log10(root->num) / log10(2);

	return (u32)height;
}

typedef enum {
	DELETE_TERMINATE = 0,
	RED_NO_CHILDREN,
	BLACK_ONE_CHILD,
	BLACK_NO_CHILDREN,
} rbtree_delete_t;

typedef enum {
	RED_SIBLING,
	BLACK_SIBLING_WITH_BLACK_CHILDREN,
	BLACK_SIBLING_ONE_RED_CHILD,
	
	BLACK_OR_NO_FAR_NEPHEW,
} rbtree_delete_balance_t;

static void rbtree_dump_node(struct rbtree *tree, FILE *stream)
{
	if (tree == NULL) {
		printf("null");
		return;
	}

	printf("d:[");
	printf("%llu,%s,%llu", (unsigned long long)tree->key, 
			test_bit(RBTREE_RED_FLAG, &tree->flags) ? 
			"RED" : "BLACK", tree->parent ? 
			(unsigned long long)tree->parent->key : 0ULL);
	
	printf("]");

	if (tree->left != NULL) {
		printf("l:[");
		rbtree_dump_node(tree->left, stream);
		printf("]");
	}

	if (tree->right != NULL) {
		printf("r:[");
		rbtree_dump_node(tree->right, stream);
		printf("]");
	}
}

void rbtree_dump(struct rbtree_root *root, FILE *stream)
{
	rbtree_dump_node(root->tree, stream);
	fputc('\n', stream);
}

