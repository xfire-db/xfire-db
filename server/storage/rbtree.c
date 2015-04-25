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
#include <assert.h>

#include <xfire/xfire.h>
#include <xfire/types.h>
#include <xfire/flags.h>
#include <xfire/bitops.h>
#include <xfire/rbtree.h>
#include <xfire/os.h>

static void __rbtree_insert(struct rbtree_root *root, struct rbtree *new);
static inline struct rbtree *rb_acquire_area_rr(struct rbtree_root *root,
					     struct rbtree *gp,
					     struct rbtree *p,
					     struct rbtree *n);
static inline void rb_release_area_rr(struct rbtree *gp, struct rbtree *p,
					struct rbtree *n);
static inline struct rbtree *rbtree_grandparent(struct rbtree *node);

void rbtree_init_node(struct rbtree *node)
{
	xfire_mutex_init(&node->lock);
	xfire_cond_init(&node->condi);
	atomic_flags_init(&node->flags);
	atomic_init(&node->ldepth);

	node->next = NULL;
	node->prev = NULL;
}

void rbtree_init_root(struct rbtree_root *root)
{
	xfire_spinlock_init(&root->lock);
	atomic_flags_init(&root->flags);
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
	atomic_inc(node->ldepth);
	return;
}

static void rbtree_unlock_node(struct rbtree *node)
{
	if(!node)
		return;

	xfire_mutex_unlock(&node->lock);
	atomic_dec(node->ldepth);
}


#ifdef HAVE_NO_RECURSION
static bool raw_rbtree_search(struct rbtree *tree, u64 key, struct rbtree **rv)
{
	struct rbtree *next;

	if(!tree) {
		*rv = NULL;
		return false;
	}

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
static struct rbtree *__rbtree_search(struct rbtree *tree, u64 key)
{
	bool again = true;
	struct rbtree *node;

	for(; again;) {
		again = raw_rbtree_search(tree, key, &node);
	}

	return node;
}
#else
static bool raw_rbtree_search(struct rbtree *tree, u64 key, struct rbtree **rv)
{
	if(!tree)
		return false;

	for(;;) {

		if(!tree) {
			return false;
		}

		if(tree->key == key) {
			*rv = tree;
			return false;
		}

		if(key < tree->key)
			tree = tree->left;
		else
			tree = tree->right;
	}

	return false;
}
static struct rbtree *__rbtree_search(struct rbtree *tree, u64 key)
{
	bool again = true;
	struct rbtree *node = NULL;

	while(again) {
		again = raw_rbtree_search(tree, key, &node);
	}

	return node;
}
#endif

struct rbtree *rbtree_find(struct rbtree_root *root, u64 key)
{
	struct rbtree *rn = rbtree_get_root(root);
	return __rbtree_search(rn, key);
}

struct rbtree *rbtree_find_duplicate(struct rbtree_root *root, u64 key,
				     bool (*cmp)(struct rbtree*, const void*),
				     const void *arg)
{
	struct rbtree *node, *c,
		      *gp,
		      *p;
	bool done = false;

	do {
		node = __rbtree_search(rbtree_get_root(root), key);

		if(!node)
			return NULL;

		gp = rbtree_grandparent(node);
		p = node->parent;

		if(rb_acquire_area_rr(root, gp, p, node))
			done = true;
	} while(!done);

	if(cmp(node, arg))
		return node;

	c = node;
	for(c = c->next; c; c = c->next) {
		if(cmp(c, arg))
			return c;
	}

	rb_release_area_rr(gp, p, node);

	return NULL;
}

static void rbtree_lpush(struct rbtree *head, struct rbtree *node)
{
	if(!head->next) {
		head->next = node;
		node->prev = head;
		node->next = NULL;
	} else {
		/* head already has duplicates */
		node->next = head;
		node->prev = NULL;

		head->prev = node;
	}
}

static void rbtree_pop(struct rbtree *node)
{
	if(node->prev)
		node->prev->next = node->next;
	if(node->next)
		node->next->prev = node->prev;

	node->next = node->prev = NULL;
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
		bool (*cmp)(struct rbtree*, const void*), const void *arg)
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
	struct rbtree *entry,
		      *gp,
		      *p;
	bool done = false;

	do {
		entry = rbtree_insert(root, node);
		if(!entry)
			return NULL;

		gp = rbtree_grandparent(entry);
		p = entry->parent;

		if(rb_acquire_area_rr(root, gp, p, entry))
			done = true;

	} while(!done);

	if(entry != node)
		rbtree_lpush(entry, node);

	rb_release_area_rr(gp, p, entry);
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
	bool again = false;

	do {
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
	} while(again);

	return node;
}

typedef enum {
	INSERT_SUCCESS,
	INSERT_RETRY,
} rbtree_insert_t;

static void rb_balance(struct rbtree_root *root, struct rbtree *node);
static struct rbtree *rbtree_balance_rr(struct rbtree_root *root,
					struct rbtree      *node);
static void rbtree_fixup_bt(struct rbtree_root *root, struct rbtree *node)
{
	struct rbtree *rn;

	rn = rbtree_get_root(root);
	while(node && node != rn && rb_red(node) && !rb_unlinked(node)) {
		node = rbtree_balance_rr(root, node);
		rn = rbtree_get_root(root);
	
		if(node && (!rb_red(node->left) && !rb_red(node->right))) {
			break;
		}
	}

	rb_set_blk(rn);
}

static inline struct rbtree *rb_push_red(struct rbtree *p, struct rbtree *n)
{
	bool dblk = rb_dblk(p);

	rb_set_blk(n);
	if(dblk) {
		rb_set_blk(p);
		return n;
	} else {
		rb_set_red(p);
		return p->parent;
	}

}

static struct rbtree *raw_rbtree_balance_rr(struct rbtree_root *root,
					    struct rbtree *node)
{
	struct rbtree *p,
		      *s,
		      *tmp = NULL,
		      *orig = node;
	bool extra_lock = false;

	p = node->parent;
	s = rbtree_sibling(node);

	if(rb_red(p) || !rb_red(node))
		return node;
	else if(!rb_red(node->left) && !rb_red(node->right))
		return node;

	if(s && test_and_clear_bit(RBTREE_RED_FLAG, &s->flags))
		return rb_push_red(p, node);

	if(p->left == node && rb_red(node->right)) {
		/* rotate left */
		tmp = node->right;
		rbtree_lock_node(tmp);
		rbtree_rotate_left(root, node);
		node = node->parent;
		extra_lock = true;
	} else if(p->right == node && rb_red(node->left)) {
		/* rotate right */
		tmp = node->left;
		rbtree_lock_node(tmp);
		rbtree_rotate_right(root, node);
		node = node->parent;
		extra_lock = true;
	}

	p = node->parent;
	if(p->left == node) {
		if(!tmp)
			tmp = node->left;
		rbtree_rotate_right(root, p);
	} else {
		if(!tmp)
			tmp = node->right;
		rbtree_rotate_left(root, p);
	}

	if(test_and_clear_bit(RBTREE_DBLK_FLAG, &p->flags)) {
		rb_set_blk(p);
		rb_set_blk(orig);
		rb_set_blk(tmp);
	} else {
		/* swap colors between n and pre-rotation parent */
		swap_bit(RBTREE_RED_FLAG, &node->flags, &p->flags);
	}

	if(extra_lock)
		rbtree_unlock_node(tmp);
	return node;
}

static inline struct rbtree *rb_acquire_area_rr(struct rbtree_root *root,
					     struct rbtree *gp,
					     struct rbtree *p,
					     struct rbtree *n)
{
	rbtree_lock_node(gp);
	if(gp != rbtree_grandparent(n) || rb_unlinked(gp)) {
		rbtree_unlock_node(gp);
		return NULL;
	}

	rbtree_lock_node(p);
	if(p != n->parent || rb_unlinked(p)) {
		rbtree_unlock_node(p);
		rbtree_unlock_node(gp);
		return NULL;
	}

	rbtree_lock_node(n);
	if(rb_unlinked(n)) {
		rbtree_unlock_node(n);
		rbtree_unlock_node(p);
		rbtree_unlock_node(gp);
		return NULL;
	}
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
		rb_balance(root, p);
		return node;
	}

	if(!rb_acquire_area_rr(root, gp, p, node))
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

	if(!rb_acquire_area_rr(root, gp, parent, node))
		return rv;

	if(rb_unlinked(node))
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
	rbtree_insert_t update = INSERT_RETRY;
	
	do {
		node = rbtree_find_insert(root, new);
		if(!node) {
			rbtree_lock_root(root);
			if(!root->tree) {
				root->tree = new;
				rb_set_blk(new);
				rbtree_unlock_root(root);
				return;
			} else {
				update = INSERT_RETRY;
			}
			rbtree_unlock_root(root);
		}
		update = rbtree_attempt_insert(root, &node, new);
	} while(update == INSERT_RETRY);

	rb_balance(root, node);
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
	if(!tree || !tree->left)
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

static struct rbtree *rbtree_find_replacement(struct rbtree *node)
{
	struct rbtree *rv;

	rv = rbtree_find_leftmost(node);

	if(!rv) {
		return rbtree_find_leftmost(node->right);
	}
	
	return rv->right ? rv->right : rv;
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

	atomic_flags_copy(&replacement->flags, &orig->flags);
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
	return node->next != NULL;
}

static bool __rbtree_remove_duplicate(struct rbtree_root *root,
				      struct rbtree      *node,
				      const void *arg)
{
	struct rbtree *replacement, *tmp,
		      *gp, *p;

	if(!node)
		return false;

	gp = rbtree_grandparent(node);
	p = node->parent;

	if(!rb_acquire_area_rr(root, gp, p, node))
		return false;

	if(!rbtree_node_has_duplicates(node)) {
		rb_release_area_rr(gp, p, node);
		return false;
	}

	if(root->iterate(node, arg)) {
		/* replace *node* with node::duplicates::next */
		replacement = node->next;
		
		rbtree_lock_node(replacement);
		tmp = node->next ? node->next->next : NULL;
		rbtree_pop(replacement);
		replacement->next = tmp;

		rbtree_replace_node(root, node, replacement);
		set_bit(RBTREE_UNLINKED_FLAG, &node->flags);

		rbtree_unlock_node(replacement);
		rb_release_area_rr(gp, p, node);
		return true;
	}

	tmp = node;
	for(tmp = tmp->next; tmp; tmp = tmp->next) {
		if(root->iterate(tmp, arg)) {
			rbtree_pop(tmp);
			break;
		}
	}

	rb_release_area_rr(gp, p, node);
	return true;
}

typedef enum {
	REMOVE_TERMINATE = 0,
	RED_NO_CHILDREN,
	BLACK_ONE_CHILD,
	BLACK_NO_CHILDREN,
	REMOVE_AGAIN,
} rbtree_delete_t;

static struct rbtree dummy_bb_node;
#define RB_BALANCED (&dummy_bb_node)

static struct rbtree *rb_push_black(struct rbtree *p, struct rbtree *n,
					struct rbtree *s)
{
	struct rbtree *sl, *sr;

	assert(p != NULL);
	assert(s != NULL);

	sl = s->left;
	sr = s->right;

	if(rb_dblk(s)) {
		if(rb_dblk(p))
			return n;

		rb_set_blk(s);
		rb_set_blk(n);
		if(rb_red(p)) {
			rb_set_blk(p);
			return RB_BALANCED;
		} else {
			rb_set_dblk(p);
			return p;
		}
	} else if(!rb_red(sl) && !rb_red(sr)) {
		if(!rb_dblk(p)) {
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
		} else {
			return n;
		}
	}

	return NULL;
}

static struct rbtree *raw_rb_resolve_nephew(struct rbtree_root *root,
					    struct rbtree *p,
					    struct rbtree *n,
					    struct rbtree *s,
					    struct rbtree *fn,
					    struct rbtree *cn)
{
	assert(p != NULL);
	assert(s != NULL);

	if(rb_dblk(p))
		return n;

	if(rb_red(cn) && rb_blk(fn)) {
		if(fn == s->left) {
			rbtree_rotate_left(root, s);
			s = s->parent;
			fn = s->left;
		} else {
			rbtree_rotate_right(root, s);
			s = s->parent;
			fn = s->right;
		}
	}

	rb_set_blk(fn);

	if(test_and_clear_bit(RBTREE_RED_FLAG, &p->flags))
		rb_set_red(s);
	else
		rb_set_blk(s);

	if(p->right == s)
		rbtree_rotate_left(root, p);
	else
		rbtree_rotate_right(root, p);

	if(rb_dblk(n)) {
		rb_set_blk(n);
	} else {
		rb_set_red(n);
	}

	return RB_BALANCED;
}

static struct rbtree *rb_resolve_nephew(struct rbtree_root *root,
					struct rbtree *p,
					struct rbtree *n, struct rbtree *s,
					struct rbtree *fn, bool ndir)
{
	struct rbtree *tmp = NULL, *cn, *rv;

	assert(p != NULL);
	assert(s != NULL);
	
	if(fn == s->right)
		cn = s->left;
	else
		cn = s->right;

	if(rb_red(cn) && rb_blk(fn))
		tmp = ndir ? s->left : s->right;

	if(tmp)
		rbtree_lock_node(tmp);

	rv = raw_rb_resolve_nephew(root, p, n, s, fn, cn);

	if(tmp)
		rbtree_unlock_node(tmp);

	return rv;
}

static struct rbtree *rb_resolve_black_node(struct rbtree_root *root,
						struct rbtree *p,
						struct rbtree *n,
						struct rbtree *s,
						bool ndir)
{
	struct rbtree *sl, *sr;

	assert(p != NULL);
	assert(s != NULL);

	sl = s->left;
	sr = s->right;

	if(rb_blk(sl) && rb_blk(sr) && (rb_blk(s) || rb_dblk(s)))
		return rb_push_black(p, n, s);
	else
		return rb_resolve_nephew(root, p, n, s, ndir ? sr : sl, ndir);
}

static struct rbtree *rb_resolve_red_sibling(struct rbtree_root *root,
					     struct rbtree *p,
					     struct rbtree *n,
					     struct rbtree *s,
					     bool ndir)
{
	struct rbtree *ns;

	assert(s != NULL);
	assert(p != NULL);

	ns = ndir ? s->left : s->right;
	assert(ns != NULL);

	if(rb_dblk(p))
		return n;

	rbtree_lock_node(ns);
	if(rb_red(ns) || rb_red(p)) {
		goto err_l;
	} else {
		/* p = black */
		rb_set_blk(s);
		rb_set_red(p);
	}

	if(ndir)
		rbtree_rotate_left(root, p);
	else
		rbtree_rotate_right(root, p);

	n = rb_resolve_black_node(root, p, n, ns, ndir);
err_l:
	rbtree_unlock_node(ns);
	return n;
}

static inline void rb_release_area_bb(struct rbtree *gp,
				      struct rbtree *p,
				      struct rbtree *n,
				      struct rbtree *s)
{
	rbtree_unlock_node(s);
	rb_release_area_rr(gp, p, n);
}

static inline bool rb_acquire_area_bb(struct rbtree_root *root,
				      struct rbtree *gp,
				      struct rbtree *p,
				      struct rbtree *n,
				      struct rbtree *s)
{
	if(!rb_acquire_area_rr(root, gp, p, n))
		return false;

	rbtree_lock_node(s);
	if(rb_unlinked(s)) {
		rbtree_unlock_node(s);
		rb_release_area_rr(gp, p, n);
		return false;
	}

	return true;
}

static struct rbtree *raw_rb_balance_bb(struct rbtree_root *root,
					struct rbtree *p,
					struct rbtree *node,
					struct rbtree *s)
{

	struct rbtree *sr, *sl,
		      *rv;
	bool ndir;

	assert(s != NULL);
	assert(p != NULL);

	sr = s->right;
	sl = s->left;
	ndir = p->left == node;

	if(!test_bit(RBTREE_REMOVE_FLAG, &node->flags) && !rb_dblk(node))
		return node;

	if(rb_dblk(p))
		return node;

	if(rb_red(s)) {
		if(rb_red(ndir ? s->left : s->right)) {
			return node;
		} else if(rb_red(p)) {
			return node;
		}
	}

	if(rb_red(s))
		rv = rb_resolve_red_sibling(root, p, node, s, ndir);
	else if((rb_dblk(s) || rb_blk(s)) && (rb_blk(sl) && rb_blk(sr)))
		rv = rb_push_black(p, node, s);
	else
		rv = rb_resolve_nephew(root, p, node, s, ndir ? sr : sl, ndir);

	return rv;
}

static void rb_fixup_bb(struct rbtree_root *root, struct rbtree *node);
static struct rbtree *rb_balance_bb(struct rbtree_root *root,
				    struct rbtree *node)
{
	struct rbtree *gp, *p, *s;
	bool ndir;
	struct rbtree *rv;

	gp = rbtree_grandparent(node);
	p = node->parent;
	s = rbtree_sibling(node);

	assert(p != NULL);
	assert(s != NULL);

	if(!test_bit(RBTREE_REMOVE_FLAG, &node->flags) && !rb_dblk(node))
		return node;

	if(rb_dblk(p)) {
		rb_balance(root, p);
		return node;
	}

	ndir = p->left == node;

	if(rb_red(s)) {
		if(rb_red(ndir ? s->left : s->right)) {
			rb_balance(root, ndir ? s->left : s->right);
			return node;
		} else if(rb_red(p)) {
			rb_balance(root, p);
			return node;
		}
	}

	if(!rb_acquire_area_bb(root, gp, p, node, s))
		return node;

	rv = raw_rb_balance_bb(root, p, node, s);

	rb_release_area_bb(gp, p, node, s);

	return rv;
}


static void rb_fixup_bb(struct rbtree_root *root, struct rbtree *node)
{
	struct rbtree *rn;

	rn = rbtree_get_root(root);
	while(node && node != RB_BALANCED && rb_dblk(node) && node != rn &&
			!rb_unlinked(node)) {
		node = rb_balance_bb(root, node);
		rn = rbtree_get_root(root);
	}

	rb_set_blk(rn);
}

static void rb_balance(struct rbtree_root *root, struct rbtree *node)
{
	if(rb_red(node))
		rbtree_fixup_bt(root, node);
	else if(rb_dblk(node))
		rb_fixup_bb(root, node);
}

static inline void rb_remove_node(struct rbtree *parent, struct rbtree *node)
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
	
	set_bit(RBTREE_UNLINKED_FLAG, &node->flags);
}

static bool rb_attempt_remove_blk_leaf(struct rbtree_root *root,
					   struct rbtree *node,
					   struct rbtree **imbalance)
{
	struct rbtree *p, *s, *rn;
	struct rbtree *result;

	p = node->parent;
	s = rbtree_sibling(node);

	rbtree_lock_root(root);
	rn = root->tree;
	if(node == rn && !node->left && !node->right) {
		rb_remove_node(NULL, node);
		root->tree = NULL;

		*imbalance = RB_BALANCED;
		return true;
	}
	rbtree_unlock_root(root);

	if(rb_unlinked(node) || (!node->left && !node->right && !node->parent))
		return false;

	set_bit(RBTREE_REMOVE_FLAG, &node->flags);
	result = raw_rb_balance_bb(root, p, node, s);
	clear_bit(RBTREE_REMOVE_FLAG, &node->flags);

	if(!result || result == node) {
		return false;
	} else {
		rb_remove_node(p, node);
	}

	*imbalance = result;
	return true;
}

static rbtree_delete_t rbtree_do_remove(struct rbtree_root *root,
					struct rbtree *current,
					struct rbtree **imbalance,
					rbtree_delete_t action)
{
	struct rbtree *parent = current->parent,
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
		set_bit(RBTREE_UNLINKED_FLAG, &current->flags);
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
		set_bit(RBTREE_UNLINKED_FLAG, &current->flags);
		break;

	default:
		break;
	}

	return action;
}

struct rbtree *raw_rb_remove(struct rbtree_root *root, struct rbtree *node,
		struct rbtree *orig)
{
	struct rbtree *gp, *p, *s,
		      *replacement = NULL,
		      *balance = RB_BALANCED;
	rbtree_delete_t action;

	gp = rbtree_grandparent(node);
	p = node->parent;
	s = rbtree_sibling(node);

	if(!rb_acquire_area_bb(root, gp, p, node, s))
		return NULL;

	if(rb_dblk(node) || rb_unlinked(node)) {
		rb_release_area_bb(gp, p, node, s);
		return NULL;
	}

	if(orig) {
		if(node != rbtree_find_replacement(orig) ||
				!(node->left && node->right)) {
			rb_release_area_bb(gp, p, node, s);
			return NULL;
		}

	}

	if(node->left && node->right) {
		replacement = rbtree_find_replacement(node);
		set_bit(RBROOT_BUSY_FLAG, &root->flags);

		if(raw_rb_remove(root, replacement, orig) != replacement) {
			clear_bit(RBROOT_BUSY_FLAG, &root->flags);
			rb_release_area_bb(gp, p, node, s);
			return NULL;
		} else {
			rbtree_lock_node(replacement);
			rbtree_replace_node(root, node, replacement);
			rbtree_unlock_node(replacement);
			clear_bit(RBROOT_BUSY_FLAG, &root->flags);
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

		action = rbtree_do_remove(root, node, &balance, action);
		if(action == REMOVE_AGAIN) {
			rb_release_area_bb(gp, p, node, s);
			return NULL;
		} else if(balance != RB_BALANCED && balance) {
			rb_release_area_bb(gp, p, node, s);
			rb_balance(root, balance);
			return node;
		}
	}

	rb_release_area_bb(gp, p, node, s);
	return node;
}

struct rbtree *rbtree_remove(struct rbtree_root *root,
			     u64 key,
			     const void *arg)
{
	struct rbtree *find;
	bool done = false;

	do {
		while(test_bit(RBROOT_BUSY_FLAG, &root->flags));

		find = rbtree_find(root, key);
		if(!find) {
			return NULL;
		}

		if(rb_dblk(find) || rb_unlinked(find))
			continue;

		if(rbtree_node_has_duplicates(find)) {
			 done =__rbtree_remove_duplicate(root, find, arg);
		} else {
			if(raw_rb_remove(root, find, NULL))
				done = true;
		}
	} while(!done);


	return find;
}

static const char *colours[] = {
	"BLACK", "RED", "DOUBLE BLACK"
};

static inline const char *rb_colour_to_string(struct rbtree *node)
{
	if(rb_blk(node))
		return colours[0];
	else if(rb_red(node))
		return colours[1];
	else
		return colours[2];
}

static void rbtree_dump_node(struct rbtree *tree, FILE *stream)
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

