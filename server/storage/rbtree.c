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
#include <xfire/flags.h>
#include <xfire/bitops.h>
#include <xfire/rbtree.h>
#include <xfire/os.h>

static void __rbtree_insert(struct rbtree_root *root, struct rbtree *new);

void rbtree_init_node(struct rbtree *node)
{
	xfire_mutex_init(&node->lock);
	xfire_cond_init(&node->condi);
	atomic_flags_init(&node->flags);

	node->duplicates.next = NULL;
	node->duplicates.prev = NULL;
}

void rbtree_init_root(struct rbtree_root *root)
{
	xfire_spinlock_init(&root->lock);
}

static inline void rbtree_lock_root(struct rbtree_root *root)
{
	xfire_spin_lock(&root->lock);
}

static inline void rbtree_unlock_root(struct rbtree_root *root)
{
	xfire_spin_unlock(&root->lock);
}

static void rbtree_lock_node(struct rbtree *node)
{
	if(!node)
		return;

	xfire_mutex_lock(&node->lock);
	set_bit(RBTREE_LOCKED_FLAG, &node->flags);
	return;
}

static void rbtree_unlock_node(struct rbtree *node)
{
	if(!node)
		return;
	else if(!test_and_clear_bit(RBTREE_LOCKED_FLAG, &node->flags))
		return;

	xfire_mutex_unlock(&node->lock);
}


#ifndef HAVE_NO_RECURSION
static bool raw_rbtree_search(struct rbtree *tree, u64 key, struct rbtree **rv)
{
	struct rbtree *next;

	if(!tree) {
		*rv = NULL;
		return false;
	}

	if(test_bit(RBTREE_IGNORE_FLAG, &tree->flags))
		return true;

	if(tree->key == key) {
		*rv = tree;
		return false;
	}

	if(key < tree->key)
		next = tree->left;
	else
		next = tree->right;

	return raw_rbtree_search(next, key, rv);
}
static struct rbtree *__rbtree_search(struct rbtree *tree, u64 key, u32 h)
{
	bool again = true;
	struct rbtree *node;

	for(; again;) {
		again = raw_rbtree_search(tree, key, &node);
	}

	return node;
}
#else
static struct rbtree *__rbtree_search(struct rbtree *tree, u64 key, u32 h)
{
	u32 depth;
	struct rbtree *rv = NULL,
		      *tmp;

	if(!tree)
		return NULL;

	for(depth = 0UL; depth < (h+1); depth++) {
		tmp = tree;

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
	struct rbtree *rn = rbtree_get_root(root);
	return __rbtree_search(rn, key, root->height);
}

struct rbtree *rbtree_find_duplicate(struct rbtree_root *root, u64 key,
				     bool (*cmp)(struct rbtree*,void*),
				     void *arg)
{
	struct rbtree *node;
	struct list_head *c;

	node = __rbtree_search(rbtree_get_root(root), key, root->height);

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

struct rbtree *__rbtree_get_node(struct rbtree *node)
{
	if(!node)
		return NULL;

	rbtree_lock_node(node);
	while(test_bit(RBTREE_ACQUIRED_FLAG, &node->flags))
		xfire_cond_wait(&node->condi, &node->lock);
	rbtree_unlock_node(node);

	return node;
}

struct rbtree *rbtree_get_node(struct rbtree_root *root, u64 key,
		bool (*cmp)(struct rbtree*, void*), void *arg)
{
	struct rbtree *node;

	node = rbtree_find_duplicate(root, key, cmp, arg);
	if(!node)
		return NULL;

	return __rbtree_get_node(node);
}

void rbtree_put_node(struct rbtree *node)
{
	if(!node)
		return;

	rbtree_lock_node(node);
	clear_bit(RBTREE_ACQUIRED_FLAG, &node->flags);
	xfire_cond_signal(&node->condi);
	rbtree_unlock_node(node);
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

	if(!rbtree_get_root(root)) {
		rbtree_lock_root(root);
		if(!root->tree) {
			root->tree = node;
			rbtree_unlock_root(root);
			return node;
		}
		rbtree_unlock_root(root);
	}

	set_bit(RBTREE_RED_FLAG, &node->flags);
	__rbtree_insert(root, node);

	return node;
}

struct rbtree *rbtree_insert_duplicate(struct rbtree_root *root,
				       struct rbtree      *node)
{
	struct rbtree *entry;

	entry = rbtree_insert(root, node);
	rbtree_lock_node(node);
	if(entry != node) {
		rbtree_lpush(entry, node);
		set_bit(RBTREE_HAS_DUPLICATES_FLAG, &entry->flags);
	}
	rbtree_unlock_node(node);

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
		rbtree_set_root(root, left);
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
		rbtree_set_root(root, right);
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

#define should_rotate_left(__n) (!rbtree_node_is_right(__n) && \
				!test_bit(RBTREE_RED_FLAG, &__n->left->flags))
#define should_rotate_right(__n) (rbtree_node_is_right(__n) && \
				test_bit(RBTREE_RED_FLAG, &__n->left->flags))

static struct rbtree *rbtree_find_insert(struct rbtree_root *root,
		struct rbtree *node)
{
	struct rbtree *tree,
		      *tmp = NULL,
		      *rn;

	tree = rbtree_get_root(root);
	for(;;) {
		rn = rbtree_get_root(root);
		if(!tree)
			return (tmp == rn) ? 
				rn : tmp;

		tmp = tree;
		if(node->key <= tree->key)
			tree = tree->left;
		else
			tree = tree->right;
	}

	return node;
}

typedef enum {
	INSERT_SUCCESS,
	INSERT_RETRY,
} rbtree_insert_t;

static struct rbtree *rbtree_balance_rr(struct rbtree_root *root,
					struct rbtree      *node);
static void rbtree_fixup_bt(struct rbtree_root *root, struct rbtree *node)
{
	struct rbtree *rn;

	rn = rbtree_get_root(root);
	while(node && node != rn && rb_red(node)) {
		node = rbtree_balance_rr(root, node);
		rn = rbtree_get_root(root);
	
		if(node && (!rb_red(node->left) && !rb_red(node->right))) {
			break;
		}
	}

	clear_bit(RBTREE_RED_FLAG, &rn->flags);
}

static struct rbtree *raw_rbtree_balance_rr(struct rbtree_root *root,
					    struct rbtree *node)
{
	struct rbtree *gp,
		      *p,
		      *s,
		      *tmp = NULL;

	gp = rbtree_grandparent(node);
	p = node->parent;
	s = rbtree_sibling(node);

	if(rb_red(p) || !rb_red(node)) {
		rbtree_unlock_node(node);
		return node;
	} else if(!rb_red(node->left) && !rb_red(node->right)) {
		rbtree_unlock_node(node);
		return node;
	}

	if(s && test_and_clear_bit(RBTREE_RED_FLAG, &s->flags)) {
		clear_bit(RBTREE_RED_FLAG, &node->flags);
		set_bit(RBTREE_RED_FLAG, &node->parent->flags);
		rbtree_unlock_node(node);
		return gp;
	}

	if(p->left == node && rb_red(node->right)) {
		/* rotate left */
		tmp = node->right;
		rbtree_lock_node(tmp);
		rbtree_rotate_left(root, node);
		node = node->parent;
	} else if(p->right == node && rb_red(node->left)) {
		/* rotate right */
		tmp = node->left;
		rbtree_lock_node(tmp);
		rbtree_rotate_right(root, node);
		node = node->parent;
	}

	p = node->parent;
	if(p->left == node) {
		if(!tmp) {
			tmp = p->right;
			rbtree_lock_node(tmp);
		}
		rbtree_rotate_right(root, p);
	} else {
		if(!tmp) {
			tmp = p->left;
			rbtree_lock_node(tmp);
		}
		rbtree_rotate_left(root, p);
	}
	/* swap colors between n and pre-rotation parent */
	swap_bit(RBTREE_RED_FLAG, &node->flags, &p->flags);

	rbtree_unlock_node(tmp);
	return node;
}

static inline struct rbtree *rb_acquire_area_rr(struct rbtree *gp,
					     struct rbtree *p,
					     struct rbtree *n)
{
	rbtree_lock_node(gp);
	if(gp != rbtree_grandparent(n)) {
		rbtree_unlock_node(gp);
		return NULL;
	}

	rbtree_lock_node(p);
	if(p != n->parent) {
		rbtree_unlock_node(p);
		rbtree_unlock_node(gp);
		return NULL;
	}

	rbtree_lock_node(n);

	return n;
}

static inline void rb_release_area_rr(struct rbtree *gp, struct rbtree *p,
				   struct rbtree *n)
{
	rbtree_unlock_node(n);
	rbtree_unlock_node(p);
	rbtree_unlock_node(gp);
}

static struct rbtree *rbtree_balance_rr(struct rbtree_root *root,
					struct rbtree      *node)
{
	struct rbtree *rv = node;
	struct rbtree *gp = rbtree_grandparent(node),
		      *p = node->parent,
		      *rn = rbtree_get_root(root);

	if((p && test_bit(RBTREE_RED_FLAG, &p->flags)) && 
			test_bit(RBTREE_RED_FLAG, &node->flags)) {
		rbtree_fixup_bt(root, p);
		return node;
	}

	if(!rb_acquire_area_rr(gp, p, node))
		return node;

	if(rn == rbtree_get_root(root))
		rv = raw_rbtree_balance_rr(root, node);

	rb_release_area_rr(gp, p, node);

	return rv;
}

static rbtree_insert_t rbtree_attempt_insert(struct rbtree_root *root,
					     struct rbtree **_node,
					     struct rbtree *new)
{
	struct rbtree *node = *_node,
		      *parent = node->parent,
		      *gp = rbtree_grandparent(node),
		      *rn = rbtree_get_root(root);

	rbtree_insert_t rv = INSERT_RETRY;

	if(!rb_acquire_area_rr(gp, parent, node))
		return rv;

	if(rn == rbtree_get_root(root)) {
		if(new->key <= node->key && !node->left) {
			node->left = new;
			new->parent = node;
			*_node = raw_rbtree_balance_rr(root, node);
			rv = INSERT_SUCCESS;
		} else if(new->key > node->key && !node->right) {
			node->right = new;
			new->parent = node;
			*_node = raw_rbtree_balance_rr(root, node);
			rv = INSERT_SUCCESS;
		}
	}

	rb_release_area_rr(gp, parent, node);

	return rv;
}

static void __rbtree_insert(struct rbtree_root *root, struct rbtree *new)
{
	struct rbtree *node;
	rbtree_insert_t update;
	
	do {
		node = rbtree_find_insert(root, new);
		update = rbtree_attempt_insert(root, &node, new);
	} while(update == INSERT_RETRY);

	rbtree_fixup_bt(root, node);
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
	} else if(root->tree == orig) {
		root->tree = replacement;
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

static bool __rbtree_remove_duplicate(struct rbtree_root *root,
				      struct rbtree      *node,
				      void *arg)
{
	struct list_head *c;
	struct rbtree *replacement,
		      *orig;

	if(!test_bit(RBTREE_HAS_DUPLICATES_FLAG, &node->flags))
		return false;

	if(root->iterate(node, arg)) {
		/* replace *node* with node::duplicates::next */
		replacement = container_of(node->duplicates.next,
				struct rbtree, duplicates);

		rbtree_lock_node(replacement);
		replacement->duplicates.prev = NULL;
		node->duplicates.next = NULL;
		rbtree_replace_node(root, node, replacement);

		if(rbtree_node_has_duplicates(replacement)) {
			set_bit(RBTREE_HAS_DUPLICATES_FLAG, 
				&replacement->flags);
			clear_bit(RBTREE_HAS_DUPLICATES_FLAG, &node->flags);
		}

		rbtree_unlock_node(replacement);
		return true;
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

	return true;
}

typedef enum {
	REMOVE_TERMINATE = 0,
	RED_NO_CHILDREN,
	BLACK_ONE_CHILD,
	BLACK_NO_CHILDREN,
	REMOVE_AGAIN,
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

static struct rbtree *rb_acquire_area_bb(struct rbtree *gp,
					 struct rbtree *p,
					 struct rbtree *n,
					 struct rbtree *sibling,
					 struct rbtree *fn)
{
	gp = rbtree_grandparent(n);
	p = n->parent;

	rbtree_lock_node(gp);
	if(gp != rbtree_grandparent(n)) {
		rbtree_unlock_node(gp);
		return NULL;
	}

	rbtree_lock_node(p);
	if(p != n->parent) {
		rbtree_unlock_node(p);
		rbtree_unlock_node(gp);
		return NULL;
	}

	rbtree_lock_node(sibling);
	rbtree_lock_node(n);
	rbtree_lock_node(fn);
	
	return n;
}

static void rb_release_area_bb(struct rbtree *gp,
				   struct rbtree *p,
				   struct rbtree *n,
				   struct rbtree *s,
				   struct rbtree *fn)
{
	rbtree_unlock_node(fn);
	rbtree_unlock_node(n);
	rbtree_unlock_node(s);
	rbtree_unlock_node(p);
	rbtree_unlock_node(gp);
}

#define NUM_REMOVE_LOCKS_MAX 6
static void **rbtree_acquire_remove_locks(struct rbtree *node,
					struct rbtree *sibling,
					struct rbtree *fn)
{
	struct rbtree *parent = node->parent,
		      *gparent = parent->parent;
	int idx = NUM_REMOVE_LOCKS_MAX - 1;

	void **locks = mzalloc(sizeof(*locks)*NUM_REMOVE_LOCKS_MAX);

	if(gparent) {
		rbtree_lock_node(gparent);
		locks[idx--] = gparent;
	}

	if(parent) {
		rbtree_lock_node(parent);
		locks[idx--] = parent;
	}

	if(sibling) {
		rbtree_lock_node(sibling);
		locks[idx--] = sibling;
	}

	rbtree_lock_node(node);
	locks[idx--] = node;

	if(fn) {
		rbtree_lock_node(fn);
		locks[idx--] = fn;
	}

	if(sibling_has_red_child(sibling) && (!fn || 
				!test_bit(RBTREE_RED_FLAG, &fn->flags))) {
		if(fn == sibling->left && sibling->left != node) {
			rbtree_lock_node(sibling->right);
			locks[idx--] = sibling->right;
		} else if(sibling->right != node) {
			rbtree_lock_node(sibling->left);
			locks[idx--] = sibling->left;
		}
	}

	return locks;
}

static void rbtree_release_remove_locks(void **locks)
{
	struct rbtree *node;
	int idx;

	for(idx = 0; idx < NUM_REMOVE_LOCKS_MAX; idx++) {
		node = locks[idx];
		if(!node)
			continue;

		rbtree_unlock_node(node);
	}

	free(locks);
}

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
		} else if(sibling_has_black_children(sibling)) {
			action = BLACK_SIBLING_WITH_BLACK_CHILDREN;
		} else if(sibling_has_red_child(sibling)) {
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
		      *gp = rbtree_grandparent(current),
		      *node = NULL;

	switch(action) {
	case BLACK_ONE_CHILD:
		if(parent) {
			if(parent->left == current) {
				parent->left = (current->left) ?
						current->left : current->right;
				if(parent->left) {
					parent->left->parent = parent;
					node = parent->left;
				}
			} else {
				/* current is the right child of its parent */
				parent->right = (current->left) ?
						current->left : current->right;
				if(parent->right) {
					parent->right->parent = parent;
					node = parent->right;
				}
			}

			if(node) {
				clear_bit(RBTREE_RED_FLAG, &node->flags);
			}
		} else {
			rbtree_lock_root(root);
			/* current is root */
			root->tree = (current->left) ? 
					current->left : current->right;
			clear_bit(RBTREE_RED_FLAG, &root->tree->flags);
			root->tree->parent = NULL;
			rbtree_unlock_root(root);
		}

		action = REMOVE_TERMINATE;
		break;

	case BLACK_NO_CHILDREN:
		rbtree_lock_root(root);
		if(root->tree == current) {
			root->tree = NULL;
			action = REMOVE_TERMINATE;
			rbtree_unlock_root(root);
			break;
		}
		rbtree_unlock_root(root);

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
	struct rbtree *orig = node;
	bool replace = false;
	rbtree_delete_t action = REMOVE_TERMINATE;

	do {
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
			node = rbtree_find_replacement(orig);
			replace = true;
			goto successor;
		}

		while(action && action != REMOVE_AGAIN)
			action = rbtree_do_remove(root, node, action);
	} while(action == REMOVE_AGAIN);

	if(replace) {
		rbtree_replace_node(root, orig, node);
	}

	return node;
}

struct rbtree *rbtree_remove(struct rbtree_root *root,
			     u64 key,
			     void *arg)
{
	struct rbtree *find,
		      *gp,
		      *p,
		      *rv;
	bool done = false;

	do {

		find = __rbtree_search(root->tree, key, root->height);
		if(!find)
			return NULL;

		gp = rbtree_grandparent(find);
		p = find->parent;

		if(!rb_acquire_area_rr(gp, p, find))
			continue;

		if(test_bit(RBTREE_HAS_DUPLICATES_FLAG, &find->flags)) {
			done = __rbtree_remove_duplicate(root, find, arg);
			rb_release_area_rr(gp, p, find);
			return find;
		} else {
			done = true;
		}
	} while(!done);

	rv = __rbtree_remove(root, find);
	rb_release_area_rr(gp, p, find);

	return rv;
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

