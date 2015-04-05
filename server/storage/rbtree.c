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

#ifndef HAVE_NO_RECURSION
static struct rbtree *__rbtree_search(struct rbtree *tree, u64 key, u32 h)
{
	if(!tree)
		return NULL;

	if(tree->key == key)
		return tree;

	if(key < tree->key)
		return __rbtree_search(tree->left, key, h);
	else
		return __rbtree_search(tree->right, key, h);
}
#else
static struct rbtree *__rbtree_search(struct rbtree *tree, u64 key, u32 h)
{
	u32 depth;
	struct rbtree *rv = NULL;

	if(!tree)
		return NULL;

	for(depth = 0UL; depth < (h+1); depth++) {
		if(!tree) {
			rv = NULL;
			break;
		}

		if(tree->key == key) {
			rv = tree;
			break;
		}

		if(key < tree->key)
			tree = tree->left;
		else
			tree = tree->right;
	}

	return rv;
}
#endif

struct rbtree *rbtree_find(struct rbtree_root *root, u64 key)
{
	return __rbtree_search(root->tree, key, root->height);
}

struct rbtree *rbtree_find_duplicate(struct rbtree_root *root, u64 key,
				     bool (*cmp)(struct rbtree*,void*),
				     void *arg)
{
	struct rbtree *node;
	struct list_head *c;

	node = __rbtree_search(root->tree, key, root->height);

	if(cmp(node, arg))
		return node;

	for(c = node->duplicates.next; c; c = c->next) {
		node = container_of(c, struct rbtree, duplicates);
		if(cmp(node, arg))
			return node;
	}

	return NULL;
}

static void rbtree_lpush(struct rbtree *head, struct rbtree *node)
{
	struct list_head *hdups,
			 *ndups;

	if(!head->duplicates.next) {
		head->duplicates.next = &node->duplicates;
		node->duplicates.prev = &head->duplicates;
		node->duplicates.next = NULL;
	} else {
		/* head already has duplicates */
		hdups = &head->duplicates;
		ndups = &node->duplicates;

		ndups->next = hdups->next;
		hdups->next->prev = ndups;

		hdups->next = ndups;
		ndups->prev = hdups;
	}
}

static void rbtree_pop(struct rbtree *node)
{
	struct list_head *dnode;

	dnode = &node->duplicates;

	if(dnode->prev)
		dnode->prev->next = dnode->next;
	if(dnode->next)
		dnode->next->prev = dnode->prev;

	dnode->next = NULL;
	dnode->prev = NULL;
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

struct rbtree *rbtree_insert_duplicate(struct rbtree_root *root,
				       struct rbtree      *node)
{
	struct rbtree *entry;

	entry = rbtree_insert(root, node);
	if(entry != node) {
		rbtree_lpush(entry, node);
		set_bit(RBTREE_HAS_DUPLICATES_FLAG, &entry->flags);
	}

	return entry;
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

#define should_rotate_left(__n) !rbtree_node_is_right(__n) && \
				!test_bit(RBTREE_RED_FLAG, &__n->left->flags)
#define should_rotate_right(__n) rbtree_node_is_right(__n) && \
				test_bit(RBTREE_RED_FLAG, &__n->left->flags)
static u32 rbtree_insert_balance(struct rbtree_root *root, struct rbtree *node)
{
	struct rbtree *sibling,
		      *parent;
	double height;

	if(test_bit(RBTREE_IS_ROOT_FLAG, &node->flags))
		return root->height;

	node = node->parent;
	while(!test_bit(RBTREE_IS_ROOT_FLAG, &node->flags) && 
			test_bit(RBTREE_RED_FLAG, &node->flags)) {
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
		}

		parent = node->parent;
		if(parent->right == node)
			rbtree_rotate_left(root, parent);
		else
			rbtree_rotate_right(root, parent);

		swap_bit(RBTREE_RED_FLAG, &node->flags, &parent->flags);
	}

	clear_bit(RBTREE_RED_FLAG, &root->tree->flags);
	height = log10(root->num) / log10(2);

	return (u32)height;
}


static inline struct rbtree *rbtree_far_nephew(struct rbtree *node)
{
	struct rbtree *parent,
		      *rv = NULL;

	if(node && rbtree_sibling(node)) {
		parent = node->parent;
		if(parent->left == node)
			rv = parent->right->right;
		else
			rv = parent->left->left;
	}

	return rv;
}

struct rbtree *rbtree_find_leftmost(struct rbtree *tree)
{
	if(!tree)
		return NULL;

	for(;;) {
		if(!tree->left)
			return tree;
		tree = tree->left;
	}
}

struct rbtree *rbtree_find_rightmost(struct rbtree *tree)
{
	if(!tree)
		return NULL;

	for(;;) {
		if(!tree->right)
			return tree;
		tree = tree->right;
	}
}

static struct rbtree *rbtree_find_successor(struct rbtree *tree)
{
	struct rbtree *tmp,
		      *carriage;

	if(!tree)
		return NULL;

	if(!tree->right) {
		tmp = tree;
		carriage = tree->parent;

		while(carriage && carriage->left != tmp) {
			tmp = carriage;
			carriage = carriage->parent;
		}

		return carriage;
	}

	return rbtree_find_leftmost(tree->right);
}

static struct rbtree *rbtree_find_predecessor(struct rbtree *tree)
{
	struct rbtree *tmp,
		      *carriage;

	if(!tree)
		return NULL;

	if(!tree->left) {
		tmp = tree;
		carriage = tree->parent;

		while(carriage && carriage->right != tmp) {
			tmp = carriage;
			carriage = carriage->parent;
		}

		return carriage;
	}

	return rbtree_find_rightmost(tree->left);
}

static struct rbtree *rbtree_find_replacement(struct rbtree *tree)
{
	struct rbtree *successor;

	if((successor = rbtree_find_successor(tree)) == NULL)
		return NULL;

	if(test_bit(RBTREE_RED_FLAG, &successor->flags) ||
			!(!test_bit(RBTREE_RED_FLAG, &successor->flags) &&
			successor->left == NULL && successor->right == NULL))
		return successor;
	else
		return rbtree_find_predecessor(tree);
}

static void rbtree_replace_node(struct rbtree_root *root,
				struct rbtree      *orig,
				struct rbtree      *replacement)
{
	replacement->left = orig->left;
	replacement->right = orig->right;
	replacement->parent = orig->parent;

	if(orig->left)
		orig->left->parent = replacement;

	if(orig->right)
		orig->right->parent = replacement;

	if(orig->parent) {
		if(orig->parent->left == orig)
			orig->parent->left = replacement;
		else
			orig->parent->right = replacement;
	} else if(test_bit(RBTREE_IS_ROOT_FLAG, &orig->flags)) {
		root->tree = replacement;
		set_bit(RBTREE_IS_ROOT_FLAG, &replacement->flags);
	}

	swap_bit(RBTREE_RED_FLAG, &orig->flags, &replacement->flags);
}

static void __rbtree_iterate(struct rbtree *node,
			     void (*fn)(struct rbtree *))
{
	if(!node)
		return;

	fn(node);
	__rbtree_iterate(node->left, fn);
	__rbtree_iterate(node->right, fn);
}

void rbtree_iterate(struct rbtree_root *root, void (*fn)(struct rbtree *))
{
	if(!root->tree)
		return;

	__rbtree_iterate(root->tree->left, fn);
	__rbtree_iterate(root->tree->right, fn);

	fn(root->tree);
}

static inline bool rbtree_node_has_duplicates(struct rbtree *node)
{
	return node->duplicates.next != NULL;
}

static void __rbtree_remove_duplicate(struct rbtree_root *root,
				      struct rbtree      *node,
				      void *arg)
{
	struct list_head *c;
	struct rbtree *replacement,
		      *orig;

	if(root->iterate(node, arg)) {
		/* replace *node* with node::duplicates::next */
		replacement = container_of(node->duplicates.next,
				struct rbtree, duplicates);
		replacement->duplicates.prev = NULL;
		node->duplicates.next = NULL;
		rbtree_replace_node(root, node, replacement);

		if(rbtree_node_has_duplicates(replacement)) {
			set_bit(RBTREE_HAS_DUPLICATES_FLAG, 
				&replacement->flags);
			clear_bit(RBTREE_HAS_DUPLICATES_FLAG, &node->flags);
		}
		return;
	}

	orig = node;
	for(c = node->duplicates.next; c; c = c->next) {
		node = container_of(c, struct rbtree, duplicates);
		if(root->iterate(node, arg)) {
			rbtree_pop(node);
			break;
		}
	}

	if(rbtree_node_has_duplicates(orig))
		set_bit(RBTREE_HAS_DUPLICATES_FLAG, &orig->flags);
	else
		clear_bit(RBTREE_HAS_DUPLICATES_FLAG, &orig->flags);
}

typedef enum {
	REMOVE_TERMINATE = 0,
	RED_NO_CHILDREN,
	BLACK_ONE_CHILD,
	BLACK_NO_CHILDREN,
} rbtree_delete_t;

typedef enum {
	REMOVE_BALANCE_TERMINATE = 0,
	RED_SIBLING,
	BLACK_SIBLING_WITH_BLACK_CHILDREN,
	BLACK_SIBLING_ONE_RED_CHILD,

	BLACK_SIBLING_RED_FAR_NEPHEW,
} rbtree_delete_balance_t;

#define is_red(__n) (test_bit(RBTREE_RED_FLAG, &__n->flags))

#define sibling_has_black_children(__s) \
	(!__s->left || !test_bit(RBTREE_RED_FLAG, &__s->left->flags)) && \
	(!__s->right || !test_bit(RBTREE_RED_FLAG, &__s->right->flags))

#define sibling_has_red_child(__s) \
	(!test_bit(RBTREE_RED_FLAG, &__s->flags)) && \
	((__s->left && is_red(__s->left)) || (__s->right && is_red(__s->right)))

static void rbtree_remove_balance(struct rbtree_root *root,
				  struct rbtree *current)
{
	rbtree_delete_balance_t action = REMOVE_BALANCE_TERMINATE;
	struct rbtree *sibling = rbtree_sibling(current),
		      *fn = rbtree_far_nephew(current),
		      *parent;

	if(current->parent->left == current)
		current->parent->left = NULL;
	else
		current->parent->right = NULL;

	if(!sibling)
		return;

	do {
		if(test_bit(RBTREE_RED_FLAG, &sibling->flags)) {
			action = RED_SIBLING;
		}
		else if(sibling_has_black_children(sibling)) {
			action = BLACK_SIBLING_WITH_BLACK_CHILDREN;
		}
		else if(sibling_has_red_child(sibling)) {
			if(!fn || !test_bit(RBTREE_RED_FLAG, &fn->flags))
				action = BLACK_SIBLING_ONE_RED_CHILD;
			else
				action = BLACK_SIBLING_RED_FAR_NEPHEW;
		}

		switch(action) {
		case RED_SIBLING:
			parent = sibling->parent;
			swap_bit(RBTREE_RED_FLAG, &sibling->flags,
				 &parent->flags);
			if(sibling->parent->left == sibling) {
				rbtree_rotate_right(root, sibling->parent);
				sibling = current->parent->left;
				fn = sibling->left;
			} else {
				rbtree_rotate_left(root, sibling->parent);
				sibling = current->parent->right;
				fn = sibling->right;
			}

			break;

		case BLACK_SIBLING_WITH_BLACK_CHILDREN:
			set_bit(RBTREE_RED_FLAG, &sibling->flags);
			current = sibling->parent;

			if(!test_bit(RBTREE_RED_FLAG, &current->flags)) {
				sibling = rbtree_sibling(current);
				fn = rbtree_far_nephew(current);
			} else {
				action = REMOVE_BALANCE_TERMINATE;
				clear_bit(RBTREE_RED_FLAG, &current->flags);
			}
			break;

		case BLACK_SIBLING_ONE_RED_CHILD:
			if(fn == sibling->left) {
				fn = sibling->right;
				rbtree_rotate_left(root, sibling);
				sibling = fn;
				fn = sibling->left;
			} else {
				fn = sibling->left;
				rbtree_rotate_right(root, sibling);
				sibling = fn;
				fn = sibling->right;
			}

		case BLACK_SIBLING_RED_FAR_NEPHEW:
			clear_bit(RBTREE_RED_FLAG, &fn->flags);
			parent = sibling->parent;
			if(test_and_clear_bit(RBTREE_RED_FLAG, &parent->flags))
				set_bit(RBTREE_RED_FLAG, &sibling->flags);
			else
				clear_bit(RBTREE_RED_FLAG, &sibling->flags);

			if(sibling->parent->right == sibling)
				rbtree_rotate_left(root, current->parent);
			else
				rbtree_rotate_right(root, current->parent);

		default:
			action = REMOVE_BALANCE_TERMINATE;
			break;
		}
	} while(action);
}

static rbtree_delete_t rbtree_do_remove(struct rbtree_root *root,
					struct rbtree *current,
					rbtree_delete_t action)
{
	struct rbtree *parent = current->parent,
		      *node;

	switch(action) {
	case BLACK_ONE_CHILD:
		if(parent) {
			if(parent->left == current) {
				parent->left = (current->left) ?
						current->left : current->right;
				parent->left->parent = parent;
				node = parent->left;
			} else {
				/* current is the right child of its parent */
				parent->right = (current->left) ?
						current->left : current->right;
				parent->right->parent = parent;
				node = parent->right;
			}
			clear_bit(RBTREE_RED_FLAG, &node->flags);
		} else {
			/* current is root */
			root->tree = (current->left) ? 
					current->left : current->right;
			clear_bit(RBTREE_RED_FLAG, &root->tree->flags);
			set_bit(RBTREE_IS_ROOT_FLAG, &root->tree->flags);
			clear_bit(RBTREE_IS_ROOT_FLAG, &root->tree->flags);
			root->tree->parent = NULL;
		}

		action = REMOVE_TERMINATE;
		break;

	case BLACK_NO_CHILDREN:
		if(root->tree == current) {
			clear_bit(RBTREE_IS_ROOT_FLAG, &root->tree->flags);
			root->tree = NULL;
			action = REMOVE_TERMINATE;
			break;
		}

		action = REMOVE_TERMINATE;
		rbtree_remove_balance(root, current);
		break;

	default:
	case RED_NO_CHILDREN:
		parent = current->parent;
		if(parent->right == current)
			parent->right = NULL;
		else
			parent->left = NULL;

		action = REMOVE_TERMINATE;
		break;
	}

	return action;
}

static struct rbtree *__rbtree_remove(struct rbtree_root *root,
				      struct rbtree      *node)
{
	struct rbtree *orig;
	bool replace = false;
	rbtree_delete_t action = REMOVE_TERMINATE;

	successor:
	if(!node->left && !node->right) {
		if(test_bit(RBTREE_RED_FLAG, &node->flags)) {
			action = RED_NO_CHILDREN;
		} else {
			action = BLACK_NO_CHILDREN;
		}
	} else if(!(node->left && node->right) &&
			!test_bit(RBTREE_RED_FLAG, &node->flags)) {
		/* node has at most one child (and node is black) */
		action = BLACK_ONE_CHILD;
	} else if(node->left && node->right) {
		orig = node;
		node = rbtree_find_replacement(orig);
		replace = true;
		goto successor;
	}

	while(action)
		action = rbtree_do_remove(root, node, action);

	if(replace)
		rbtree_replace_node(root, orig, node);

	return node;
}

struct rbtree *rbtree_remove(struct rbtree_root *root,
			     u64 key,
			     void *arg)
{
	struct rbtree *find;

	find = __rbtree_search(root->tree, key, root->height);
	if(!find)
		return NULL;

	if(test_bit(RBTREE_HAS_DUPLICATES_FLAG, &find->flags)) {
		__rbtree_remove_duplicate(root, find, arg);
		return find;
	}


	return __rbtree_remove(root, find);
}

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

