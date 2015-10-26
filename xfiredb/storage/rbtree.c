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

/**
 * @addtogroup rbtree
 * @{
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>

#include <xfire/xfire.h>
#include <xfire/types.h>
#include <xfire/flags.h>
#include <xfire/mem.h>
#include <xfire/bitops.h>
#include <xfire/rbtree.h>
#include <xfire/os.h>

static void __rb_insert(struct rb_root *root, struct rb_node *new);
static inline struct rb_node *rb_grandparent(struct rb_node *node);

/**
 * @brief Initialise a red black node.
 * @param node Red black node to initialise.
 */
void rb_init_node(struct rb_node *node)
{
	xfire_mutex_init(&node->lock);
	xfire_cond_init(&node->condi);
	atomic_flags_init(&node->flags);
	atomic_init(&node->ldepth);

	node->next = NULL;
	node->prev = NULL;
}

/**
 * @brief Destroy a red black node.
 * @param node Node which has to be destroyed.
 */
void rb_node_destroy(struct rb_node *node)
{
	xfire_mutex_destroy(&node->lock);
	xfire_cond_destroy(&node->condi);
	atomic_flags_destroy(&node->flags);
	atomic_destroy(&node->ldepth);
}

/**
 * @brief Destroy a red-black tree root.
 * @param root Root to destroy.
 */
void rb_destroy_root(struct rb_root *root)
{
	xfire_spinlock_destroy(&root->lock);
	atomic64_destroy(&root->num);
}

/**
 * @brief Initialise a red black tree.
 * @param root Red black root to initialise.
 */
void rb_init_root(struct rb_root *root)
{
	xfire_spinlock_init(&root->lock);
	atomic64_init(&root->num);
	root->tree = NULL;
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

static inline void rb_lock_root(struct rb_root *root)
{
	xfire_spin_lock(&root->lock);
}

static inline void rb_unlock_root(struct rb_root *root)
{
	xfire_spin_unlock(&root->lock);
}

static inline void rb_lock_node(struct rb_node *node)
{
	if(!node)
		return;

	xfire_mutex_lock(&node->lock);
	atomic_inc(node->ldepth);	
}

static inline void rb_unlock_node(struct rb_node *node)
{
	xfire_mutex_unlock(&node->lock);
	atomic_dec(node->ldepth);
}

#ifdef HAVE_RECURSION
static struct rb_node *raw_rb_search(struct rb_node *tree, u64 key)
{
	struct rb_node *next;

	if(!tree)
		return NULL;

	if(tree->key == key)
		return tree;

	if(key < tree->key)
		next = tree->left;
	else
		next = tree->right;

	return raw_rb_search(next, key);
}
#else
static struct rb_node *raw_rb_search(struct rb_node *tree, u64 key)
{
	for(;;) {

		if(!tree)
			return NULL;

		if(tree->key == key) {
			return tree;
		}

		if(key < tree->key)
			tree = tree->left;
		else
			tree = tree->right;
	}

	return NULL;
}
#endif

/**
 * @brief Find a key in a red black tree.
 * @param root Red black tree to search in.
 * @param key Key to search for.
 * @return Found red black tree node.
 */
struct rb_node *rb_find(struct rb_root *root, u64 key)
{
	struct rb_node *rn,
		       *rv;

	rn = rb_get_root(root);
	rv = raw_rb_search(rn, key);

	return rv;
}

/**
 * @brief Find a key including duplicates.
 * @param root Red black tree to search for.
 * @param key Key to search for.
 * @param arg Argument to pass to rb_root::cmp.
 * @return Found red black node.
 */
struct rb_node *rb_find_duplicate(struct rb_root *root, u64 key,
					const void *arg)
{
	struct rb_node *node, *c;

	node = rb_find(root, key);

	if(!node)
		return NULL;

	if(root->cmp(node, arg))
		return node;

	c = node;
	for(c = c->next; c; c = c->next) {
		if(root->cmp(c, arg))
			return c;
	}

	return NULL;
}

static void rb_lpush(struct rb_node *head, struct rb_node *node)
{
	struct rb_node *next = head->next;

	node->next = next;
	node->prev = head;

	head->next = node;
	if(next)
		next->prev = node;

	set_bit(RB_NODE_HAS_DUPLICATES_FLAG, &head->flags);
}

static void rb_pop(struct rb_node *node)
{

	if(node->prev)
		node->prev->next = node->next;
	if(node->next)
		node->next->prev = node->prev;

	node->next = node->prev = NULL;
}

/**
 * @brief Mark the node as used.
 * @param node Node to mark as in use.
 * @return The node.
 */
struct rb_node *__rb_get_node(struct rb_node *node)
{
	if(!node)
		return NULL;

	rb_lock_node(node);
	while(test_bit(RB_NODE_ACQUIRED_FLAG, &node->flags))
		xfire_cond_wait(&node->condi, &node->lock);
	rb_unlock_node(node);

	return node;
}

/**
 * @brief Mark the given node as used.
 * @param root Red black tree to search.
 * @param key Key to search for.
 * @param arg Argument to pass to struct rb_root::cmp.
 *
 * This function, in contradiction to `__rb_get_node' also checks for duplicates.
 */
struct rb_node *rb_get_node(struct rb_root *root, u64 key, const void *arg)
{
	struct rb_node *node;

	node = rb_find_duplicate(root, key, arg);
	if(!node)
		return NULL;

	return __rb_get_node(node);
}

/**
 * @brief Mark a node as unused.
 * @param node Node to decrease in usage.
 */
void rb_put_node(struct rb_node *node)
{
	if(!node)
		return;

	rb_lock_node(node);
	clear_bit(RB_NODE_ACQUIRED_FLAG, &node->flags);
	xfire_cond_signal(&node->condi);
	rb_unlock_node(node);
}

static struct rb_node *rb_insert_duplicate(struct rb_root *root,
				       struct rb_node *node);

/**
 * @brief Insert a new node.
 * @param root Red black tree to insert into.
 * @param node Node to insert.
 * @param dup Set to true if duplicates are allowed, false if not.
 * @return The inserted node.
 */
struct rb_node *rb_insert(struct rb_root *root, struct rb_node *node,
		bool dup)
{
	struct rb_node *entry;

	node->left = NULL;
	node->right = NULL;
	node->parent = NULL;

	rb_lock_root(root);
	entry = rb_find(root, node->key);
	if(entry && !dup) {
		rb_unlock_root(root);
		return entry;
	} else if(entry && dup) {
		return rb_insert_duplicate(root, node);
	}

	atomic64_inc(root->num);
	if(!rb_get_root(root)) {
		root->tree = node;
		rb_unlock_root(root);
		return node;
	}

	set_bit(RB_NODE_RED_FLAG, &node->flags);
	__rb_insert(root, node);
	rb_unlock_root(root);
	return node;
}

/**
 * @brief Get the height of the red black tree.
 * @param root Tree to get the height of.
 * @return The height of \p root.
 */
s32 rb_get_height(struct rb_root *root)
{
	s32 n, h;

	n = rb_get_size(root);
	h = log10(n+1) / log10(2);
	h *= 2;

	return h;
}

static struct rb_node *rb_insert_duplicate(struct rb_root *root,
				       struct rb_node      *node)
{
	struct rb_node *entry;

	entry = rb_find(root, node->key);

	if(entry != node)
		rb_lpush(entry, node);

	rb_unlock_root(root);
	return node;
}


static void rb_rotate_right(struct rb_root *root, struct rb_node *node)
{
	struct rb_node *left = node->left,
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
		root->tree = left;
	}
}

static void rb_rotate_left(struct rb_root *root, struct rb_node *node)
{
	struct rb_node *right = node->right,
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
		root->tree = right;
	}
}


static inline struct rb_node *rb_grandparent(struct rb_node *node)
{
	struct rb_node *parent = node->parent;

	if(parent)
		return parent->parent;
	else
		return NULL;
}

static inline struct rb_node *rb_sibling(struct rb_node *node)
{
	struct rb_node *parent = node->parent;

	if(parent) {
		if(node == parent->left)
			return parent->right;
		else
			return parent->left;
	}

	return NULL;
}

static inline bool rb_node_is_right(struct rb_node *node)
{
	if(node->parent && node->parent->right == node)
		return true;
	else
		return false;
}

#ifdef HAVE_RECURSION
static struct rb_node *raw_rb_find_insert(struct rb_node *node,
					  struct rb_node *prev,
					  u64 key)
{
	struct rb_node *tree = node;

	if(!node)
		return prev;

	if(key <= node->key)
		node = node->left;
	else
		node = node->right;

	return raw_rb_find_insert(node, tree, key);
}

static struct rb_node *rb_find_insert(struct rb_root *root,
					struct rb_node *node)
{
	struct rb_node *rn = rb_get_root(root);

	return raw_rb_find_insert(rn, NULL, node->key);
}
#else
static struct rb_node *rb_find_insert(struct rb_root *root,
		struct rb_node *node)
{
	struct rb_node *tree,
		      *tmp = NULL;

	tree = root->tree;
	for(;;) {
		if(!tree)
			return tmp;

		tmp = tree;
		if(node->key <= tree->key)
			tree = tree->left;
		else
			tree = tree->right;
	}

	return node;
}
#endif

static void rb_balance(struct rb_root *root, struct rb_node *node);
static struct rb_node *rb_balance_rr(struct rb_root *root,
					struct rb_node      *node);
static void rb_fixup_rr(struct rb_root *root, struct rb_node *node)
{
	struct rb_node *rn;

	rn = root->tree;
	while(node && node != rn && rb_red(node) && !rb_unlinked(node)) {
		node = rb_balance_rr(root, node);
		rn = root->tree;
	
		if(node && (!rb_red(node->left) && !rb_red(node->right))) {
			break;
		}
	}

	rb_set_blk(rn);
}

static struct rb_node *raw_rb_balance_rr(struct rb_root *root,
					    struct rb_node *node)
{
	struct rb_node *gp, *p, *s;

	gp = rb_grandparent(node);
	p = node->parent;
	s = rb_sibling(node);

	if(rb_red(p) || !rb_red(node))
		return node;
	else if(!rb_red(node->left) && !rb_red(node->right))
		return node;

	if(s && test_and_clear_bit(RB_NODE_RED_FLAG, &s->flags)) {
		rb_set_blk(node);
		rb_set_red(p);
		return gp;
	}

	if(p->left == node && rb_red(node->right)) {
		/* rotate left */
		rb_rotate_left(root, node);
		node = node->parent;
	} else if(p->right == node && rb_red(node->left)) {
		/* rotate right */
		rb_rotate_right(root, node);
		node = node->parent;
	}

	p = node->parent;
	if(p->left == node)
		rb_rotate_right(root, p);
	else
		rb_rotate_left(root, p);

	rb_swap_color(node, p);
	return node;
}

static struct rb_node *rb_balance_rr(struct rb_root *root,
					struct rb_node      *node)
{
	struct rb_node *p = node->parent;

	if((p && test_bit(RB_NODE_RED_FLAG, &p->flags)) && 
			test_bit(RB_NODE_RED_FLAG, &node->flags)) {
		rb_balance(root, p);
		return node;
	}

	return raw_rb_balance_rr(root, node);
}

static void rb_attempt_insert(struct rb_root *root,
			      struct rb_node *node,
			      struct rb_node *new)
{
	if(new->key <= node->key && !node->left) {
		node->left = new;
		new->parent = node;
	} else if(new->key > node->key && !node->right) {
		node->right = new;
		new->parent = node;
	}
}

static void __rb_insert(struct rb_root *root, struct rb_node *new)
{
	struct rb_node *node;
	
	node = rb_find_insert(root, new);
	rb_attempt_insert(root, node, new);

	rb_balance(root, node);
}

static inline struct rb_node *rb_far_nephew(struct rb_node *node)
{
	struct rb_node *parent,
		      *rv = NULL;

	if(node && rb_sibling(node)) {
		parent = node->parent;
		if(parent->left == node)
			rv = parent->right->right;
		else
			rv = parent->left->left;
	}

	return rv;
}

#ifdef HAVE_RECURSION
struct rb_node *rb_find_rightmost(struct rb_node *tree)
{
	struct rb_node *node;

	if(!tree)
		return NULL;

	node = tree;
	tree = tree->right;
	if(!tree)
		return node;

	return rb_find_rightmost(tree);
}

struct rb_node *rb_find_leftmost(struct rb_node *tree)
{
	struct rb_node *node;

	if(!tree)
		return NULL;

	node = tree;
	tree = tree->left;

	if(!tree)
		return node;

	return rb_find_leftmost(tree);
}
#else
/**
 * @brief Find the left mose node in a tree.
 * @param tree Node to start searching.
 * @return The left most node.
 */
struct rb_node *rb_find_leftmost(struct rb_node *tree)
{
	if(!tree)
		return NULL;

	for(;;) {
		if(!tree->left)
			return tree;

		tree = tree->left;
	}
}

/**
 * @brief Find the right most node.
 * @param tree Node to start searching in.
 * @return The right most node.
 */
struct rb_node *rb_find_rightmost(struct rb_node *tree)
{
	if(!tree)
		return NULL;

	for(;;) {
		if(!tree->right)
			return tree;

		tree = tree->right;
	}
}
#endif

static struct rb_node *rb_find_replacement(struct rb_node *node)
{
	struct rb_node *rv;

	rv = rb_find_leftmost(node);

	if(!rv)
		return rb_find_leftmost(node->right);
	
	return rv->right ? rv->right : rv;
}

static void rb_replace_node(struct rb_root *root,
				struct rb_node      *orig,
				struct rb_node      *replacement)
{
	set_bit(RB_NODE_UNLINKED_FLAG, &orig->flags);
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
	} else if(rb_get_root(root) == orig) {
		rb_set_root(root, replacement);
	}

	rb_swap_color(orig, replacement);
	clear_bit(RB_NODE_UNLINKED_FLAG, &replacement->flags);
}

static void __rb_iterate(struct rb_root *root, struct rb_node *node,
			     void (*fn)(struct rb_root *,struct rb_node *,void*), void *arg)
{
	struct rb_node *left, *right;

	if(!node)
		return;

	left = node->left;
	right = node->right;

	fn(root, node, arg);
	__rb_iterate(root, left, fn, arg);
	__rb_iterate(root, right, fn, arg);
}

struct rb_iterator *rb_new_iterator(struct rb_root *root)
{
	struct rb_iterator *it = xfire_zalloc(sizeof(*it));

	it->next = rb_get_root(root);
	it->root = root;
	return it;
}

void rb_free_iterator(struct rb_iterator *it)
{
	xfire_free(it);
}

struct rb_node *rb_iterator_next(struct rb_iterator *it)
{
	struct rb_node *rv = it->next;
	struct rb_node *tmp, *parent, *child;

	if(!rv)
		return NULL;

	rb_lock_root(it->root);
	if(rv->left) {
		tmp = rv->left;
	} else if(rv->right){
		tmp = rv->right;
	} else {
		parent = rv->parent;
		child = rv;

		while(parent && (parent->right == child
					|| parent->right == NULL)) {
			child = parent;
			parent = parent->parent;
		}

		if(!parent)
			tmp = NULL;
		else
			tmp = parent->right;
	}

	it->next = tmp;
	rb_unlock_root(it->root);

	return rv;
}

/**
 * @brief Iterate over a red black tree.
 * @param root Tree to iterate over.
 * @param fn Iteration function.
 * @param arg Argument to \p fn.
 */
void rb_iterate(struct rb_root *root, void (*fn)(struct rb_root *,struct rb_node *,void*), void *arg)
{
	struct rb_node *left, *right;

	if(!root->tree)
		return;
	
	left = root->tree->left;
	right = root->tree->right;

	__rb_iterate(root, left, fn, arg);
	__rb_iterate(root, right, fn, arg);
	fn(root, root->tree, arg);
}

typedef enum {
	REMOVE_TERMINATE = 0,
	RED_NO_CHILDREN,
	BLACK_ONE_CHILD,
	BLACK_NO_CHILDREN,
	REMOVE_AGAIN,
} rb_delete_t;

static struct rb_node dummy_bb_node;
#define RB_BALANCED (&dummy_bb_node)

static struct rb_node *rb_push_black(struct rb_node *p, struct rb_node *n,
					struct rb_node *s)
{
	struct rb_node *sl, *sr;

	assert(p != NULL);
	assert(s != NULL);

	sl = s->left;
	sr = s->right;

	if(!rb_red(sl) && !rb_red(sr)) {
		if(rb_dblk(n))
			rb_set_blk(n);
		else
			rb_set_red(n);

		rb_set_red(s);
		if(rb_blk(p)) {
			rb_set_dblk(p);
			return p;
		} else {
			rb_set_blk(p);
			return RB_BALANCED;
		}
	}

	return NULL;
}

static struct rb_node *raw_rb_resolve_nephew(struct rb_root *root,
					    struct rb_node *p,
					    struct rb_node *n,
					    struct rb_node *s,
					    struct rb_node *fn,
					    struct rb_node *cn)
{
	assert(p != NULL);
	assert(s != NULL);

	if(rb_red(cn) && rb_blk(fn)) {
		if(fn == s->left) {
			rb_rotate_left(root, s);
			s = s->parent;
			fn = s->left;
		} else {
			rb_rotate_right(root, s);
			s = s->parent;
			fn = s->right;
		}
	}

	rb_set_blk(fn);

	if(test_and_clear_bit(RB_NODE_RED_FLAG, &p->flags))
		rb_set_red(s);
	else
		rb_set_blk(s);

	if(p->right == s)
		rb_rotate_left(root, p);
	else
		rb_rotate_right(root, p);

	if(rb_dblk(n))
		rb_set_blk(n);
	else
		rb_set_red(n);

	return RB_BALANCED;
}

static struct rb_node *rb_resolve_nephew(struct rb_root *root,
					struct rb_node *p,
					struct rb_node *n, struct rb_node *s,
					struct rb_node *fn, bool ndir)
{
	struct rb_node *cn;

	assert(p != NULL);
	assert(s != NULL);
	
	if(fn == s->right)
		cn = s->left;
	else
		cn = s->right;

	return raw_rb_resolve_nephew(root, p, n, s, fn, cn);
}

static struct rb_node *rb_resolve_black_node(struct rb_root *root,
						struct rb_node *p,
						struct rb_node *n,
						struct rb_node *s,
						bool ndir)
{
	struct rb_node *sl, *sr;

	assert(p != NULL);
	assert(s != NULL);

	sl = s->left;
	sr = s->right;

	if(rb_blk(sl) && rb_blk(sr) && (rb_blk(s) || rb_dblk(s)))
		return rb_push_black(p, n, s);
	else
		return rb_resolve_nephew(root, p, n, s, ndir ? sr : sl, ndir);
}

static struct rb_node *rb_resolve_red_sibling(struct rb_root *root,
					     struct rb_node *p,
					     struct rb_node *n,
					     struct rb_node *s,
					     bool ndir)
{
	struct rb_node *ns;

	assert(s != NULL);
	assert(p != NULL);

	ns = ndir ? s->left : s->right;
	assert(ns != NULL);

	/* p = black */
	rb_set_blk(s);
	rb_set_red(p);

	if(ndir)
		rb_rotate_left(root, p);
	else
		rb_rotate_right(root, p);

	n = rb_resolve_black_node(root, p, n, ns, ndir);
	return n;
}

static bool __rb_remove_duplicate(struct rb_root *root,
					struct rb_node *node,
					struct rb_node *dup)
{
	struct rb_node *replacement;

	if(!rb_node_has_duplicates(node) || !dup) {
		clear_bit(RB_NODE_HAS_DUPLICATES_FLAG, &node->flags);
		return false;
	}

	if(test_bit(RB_NODE_ACQUIRED_FLAG, &dup->flags))
		return false;

	if(dup == node) {
		replacement = node->next;

		rb_replace_node(root, node, replacement);
		replacement->prev = NULL;

		node->left = node->right = node->parent = NULL;
		node = replacement;
	} else {
		rb_pop(dup);
	}

	if(rb_node_has_duplicates(node))
		set_bit(RB_NODE_HAS_DUPLICATES_FLAG, &node->flags);
	else
		clear_bit(RB_NODE_HAS_DUPLICATES_FLAG, &node->flags);

	return true;
}

static struct rb_node *raw_rb_balance_bb(struct rb_root *root,
					struct rb_node *p,
					struct rb_node *node,
					struct rb_node *s)
{

	struct rb_node *sr, *sl,
		      *rv;
	bool ndir;

	assert(s != NULL);
	assert(p != NULL);

	sr = s->right;
	sl = s->left;
	ndir = p->left == node;

	if(!test_bit(RB_NODE_REMOVE_FLAG, &node->flags) && !rb_dblk(node))
		return node;

	if(rb_red(s))
		rv = rb_resolve_red_sibling(root, p, node, s, ndir);
	else if((rb_dblk(s) || rb_blk(s)) && (rb_blk(sl) && rb_blk(sr)))
		rv = rb_push_black(p, node, s);
	else
		rv = rb_resolve_nephew(root, p, node, s, ndir ? sr : sl, ndir);

	return rv;
}

static void rb_fixup_bb(struct rb_root *root, struct rb_node *node);
static struct rb_node *rb_balance_bb(struct rb_root *root,
				    struct rb_node *node)
{
	struct rb_node *p, *s;

	p = node->parent;
	s = rb_sibling(node);

	assert(p != NULL);
	assert(s != NULL);

	if(!test_bit(RB_NODE_REMOVE_FLAG, &node->flags) && !rb_dblk(node))
		return node;

	return raw_rb_balance_bb(root, p, node, s);
}


static void rb_fixup_bb(struct rb_root *root, struct rb_node *node)
{
	struct rb_node *rn;

	rn = rb_get_root(root);
	while(node && node != RB_BALANCED && rb_dblk(node) && node != rn &&
			!rb_unlinked(node)) {
		node = rb_balance_bb(root, node);
		rn = rb_get_root(root);
	}

	rb_set_blk(rn);
}

static void rb_balance(struct rb_root *root, struct rb_node *node)
{
	if(rb_red(node))
		rb_fixup_rr(root, node);
	else if(rb_dblk(node))
		rb_fixup_bb(root, node);
}

static inline void rb_remove_node(struct rb_node *parent, struct rb_node *node)
{
	if(parent) {
		if(parent->left == node)
			parent->left = NULL;
		else if(parent->right == node) {
			parent->right = NULL;
		} else {
			return;
		}
	}
	
	set_bit(RB_NODE_UNLINKED_FLAG, &node->flags);
}

static bool rb_attempt_remove_blk_leaf(struct rb_root *root,
					   struct rb_node *node,
					   struct rb_node **imbalance)
{
	struct rb_node *p, *s, *rn;
	struct rb_node *result;

	p = node->parent;
	s = rb_sibling(node);

	rn = root->tree;
	if(node == rn && !node->left && !node->right) {
		rb_remove_node(NULL, node);
		root->tree = NULL;

		*imbalance = RB_BALANCED;
		return true;
	}

	if(rb_unlinked(node) || (!node->left && !node->right && !node->parent))
		return false;

	set_bit(RB_NODE_REMOVE_FLAG, &node->flags);
	result = raw_rb_balance_bb(root, p, node, s);
	clear_bit(RB_NODE_REMOVE_FLAG, &node->flags);

	if(!result || result == node) {
		return false;
	} else {
		rb_remove_node(p, node);
	}

	*imbalance = result;
	return true;
}

static rb_delete_t rb_do_remove(struct rb_root *root,
					struct rb_node *current,
					struct rb_node **imbalance,
					rb_delete_t action)
{
	struct rb_node *parent = current->parent,
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
				clear_bit(RB_NODE_RED_FLAG, &node->flags);
			}
		} else {
			/* current is root */
			root->tree = (current->left) ? 
					current->left : current->right;
			clear_bit(RB_NODE_RED_FLAG, &root->tree->flags);
			root->tree->parent = NULL;
		}

		action = REMOVE_TERMINATE;
		set_bit(RB_NODE_UNLINKED_FLAG, &current->flags);
		break;

	case BLACK_NO_CHILDREN:
		if(root->tree == current) {
			root->tree = NULL;
			action = REMOVE_TERMINATE;
			break;
		}

		if(rb_attempt_remove_blk_leaf(root, current, imbalance))
			action = REMOVE_TERMINATE;
		else
			action = REMOVE_AGAIN;
		break;

	case RED_NO_CHILDREN:
		parent = current->parent;
		if(parent->right == current)
			parent->right = NULL;
		else
			parent->left = NULL;

		action = REMOVE_TERMINATE;
		set_bit(RB_NODE_UNLINKED_FLAG, &current->flags);
		break;

	default:
		break;
	}

	return action;
}

static struct rb_node *raw_rb_remove(struct rb_root *root, struct rb_node *node,
		struct rb_node *orig)
{
	struct rb_node *replacement = NULL,
		       *balance = RB_BALANCED;
	rb_delete_t action;

	if(rb_dblk(node) || rb_unlinked(node) ||
			test_bit(RB_NODE_ACQUIRED_FLAG, &node->flags)) {
		return NULL;
	}

	if(orig) {
		if(node != rb_find_replacement(orig) || (node->left &&
					node->right)) {
			return NULL;
		}

	}

	if(node->left && node->right) {
		replacement = rb_find_replacement(node);
		if(!replacement)
			return NULL;

		if(raw_rb_remove(root, replacement, node) != replacement) {
			return NULL;
		} else {
			rb_replace_node(root, node, replacement);
			node = replacement;
		}
	} else {
		if(!node->left && !node->right) {
			if(rb_red(node))
				action = RED_NO_CHILDREN;
			else
				action = BLACK_NO_CHILDREN;
		} else {
			action = BLACK_ONE_CHILD;
		}

		action = rb_do_remove(root, node, &balance, action);

		if(action == REMOVE_AGAIN) {
			return NULL;
		} else if(balance != RB_BALANCED && balance) {
			rb_balance(root, balance);
			return node;
		}
	}

	return node;
}

/**
 * @brief Remove a red black tree node.
 * @param root Red black tree root.
 * @param key Key to search for and delete.
 * @param arg Argument to pass to struct rb_root::cmp.
 * @return The deleted node. If no node was deleted, NULL is returned.
 */
struct rb_node *rb_remove(struct rb_root *root,
			     u64 key,
			     const void *arg)
{
	struct rb_node *find, *dup;
	bool done = false;
	
	rb_lock_root(root);
	do {
		dup = find = rb_find_duplicate(root, key, arg);

		if(!find) {
			rb_unlock_root(root);
			return NULL;
		}

		while(find->prev)
			find = find->prev;

		if(rb_dblk(find) || rb_unlinked(find))
			continue;

		if(test_bit(RB_NODE_HAS_DUPLICATES_FLAG, &find->flags)) {
			done =__rb_remove_duplicate(root, find, dup);
		} else {
			if(raw_rb_remove(root, find, NULL)) {
				atomic64_dec(root->num);
				done = true;
			}
		}
	} while(!done);

	rb_unlock_root(root);
	return find;
}

static const char *colours[] = {
	"BLACK", "RED", "DOUBLE BLACK"
};

static inline const char *rb_colour_to_string(struct rb_node *node)
{
	if(rb_blk(node))
		return colours[0];
	else if(rb_red(node))
		return colours[1];
	else
		return colours[2];
}

static void rb_dump_node(struct rb_node *tree, FILE *stream)
{
	if (tree == NULL) {
		printf("null");
		return;
	}

	printf("d:[");
	printf("%llu,%s,%llu", (unsigned long long)tree->key,
			rb_colour_to_string(tree), tree->parent ? 
			(unsigned long long)tree->parent->key : 0ULL);
	
	printf("]");

	if (tree->left != NULL) {
		printf("l:[");
		rb_dump_node(tree->left, stream);
		printf("]");
	}

	if (tree->right != NULL) {
		printf("r:[");
		rb_dump_node(tree->right, stream);
		printf("]");
	}
}

/**
 * @brief Dump a red black tree to an output stream.
 * @param root Tree to dump.
 * @param stream Stream to output to.
 */
void rb_dump(struct rb_root *root, FILE *stream)
{
	rb_dump_node(root->tree, stream);
	fputc('\n', stream);
}

/** @} */

