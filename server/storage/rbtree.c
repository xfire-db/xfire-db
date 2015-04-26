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

static void __rb_insert(struct rb_root *root, struct rb_node *new);
static inline struct rb_node *rb_acquire_area_rr(struct rb_node *gp,
					     struct rb_node *p,
					     struct rb_node *n);
static inline void rb_release_area_rr(struct rb_node *gp, struct rb_node *p,
					struct rb_node *n);
static inline struct rb_node *rb_grandparent(struct rb_node *node);

void rb_init_node(struct rb_node *node)
{
	xfire_mutex_init(&node->lock);
	xfire_cond_init(&node->condi);
	atomic_flags_init(&node->flags);
	atomic_init(&node->ldepth);

	node->next = NULL;
	node->prev = NULL;
}

void rb_init_root(struct rb_root *root)
{
	xfire_spinlock_init(&root->lock);
	atomic64_init(&root->num);
}

static inline void rb_lock_root(struct rb_root *root)
{
	xfire_spin_lock(&root->lock);
}

static inline void rb_unlock_root(struct rb_root *root)
{
	xfire_spin_unlock(&root->lock);
}

static void rb_lock_node(struct rb_node *node)
{
	if(!node)
		return;

	xfire_mutex_lock(&node->lock);
	atomic_inc(node->ldepth);
	return;
}

static void rb_unlock_node(struct rb_node *node)
{
	if(!node)
		return;

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

	rb_lock_node(tree);
	if(key < tree->key)
		next = tree->left;
	else
		next = tree->right;
	rb_unlock_node(tree);

	return raw_rb_search(next, key);
}
#else
static struct rb_node *raw_rb_search(struct rb_node *tree, u64 key)
{
	struct rb_node *tmp;

	for(;;) {

		if(!tree)
			return NULL;

		if(tree->key == key)
			return tree;

		tmp = tree;
		rb_lock_node(tmp);
		if(key < tree->key)
			tree = tree->left;
		else
			tree = tree->right;
		rb_unlock_node(tmp);
	}

	return NULL;
}
#endif

struct rb_node *rb_find(struct rb_root *root, u64 key)
{
	struct rb_node *rn,
		       *rv;

	rn = rb_get_root(root);
	rv = raw_rb_search(rn, key);

	return rv;
}

struct rb_node *rb_find_duplicate(struct rb_root *root, u64 key,
					const void *arg)
{
	struct rb_node *node, *c,
		      *gp,
		      *p;
	bool done = false;

	do {
		node = rb_find(root, key);

		if(!node)
			return NULL;

		gp = rb_grandparent(node);
		p = node->parent;

		if(rb_acquire_area_rr(gp, p, node))
			done = true;
	} while(!done);

	if(root->cmp(node, arg))
		return node;

	c = node;
	for(c = c->next; c; c = c->next) {
		if(root->cmp(c, arg))
			return c;
	}

	rb_release_area_rr(gp, p, node);

	return NULL;
}

static void rb_lpush(struct rb_node *head, struct rb_node *node)
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

static void rb_pop(struct rb_node *node)
{

	if(node->prev)
		node->prev->next = node->next;
	if(node->next)
		node->next->prev = node->prev;

	node->next = node->prev = NULL;
}

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

struct rb_node *rb_get_node(struct rb_root *root, u64 key, const void *arg)
{
	struct rb_node *node;

	node = rb_find_duplicate(root, key, arg);
	if(!node)
		return NULL;

	return __rb_get_node(node);
}

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
struct rb_node *rb_insert(struct rb_root *root, struct rb_node *node,
		bool dup)
{
	struct rb_node *entry;

	node->left = NULL;
	node->right = NULL;
	node->parent = NULL;

	entry = rb_find(root, node->key);
	if(entry && !dup)
		return entry;
	else if(entry && dup)
		return rb_insert_duplicate(root, node);

	atomic64_inc(root->num);
	if(!rb_get_root(root)) {
		rb_lock_root(root);
		if(!root->tree) {
			root->tree = node;
			rb_unlock_root(root);
			return node;
		}
		rb_unlock_root(root);
	}

	set_bit(RB_NODE_RED_FLAG, &node->flags);
	__rb_insert(root, node);

	return node;
}

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
	struct rb_node *entry,
		      *gp,
		      *p;
	bool done = false;

	do {
		entry = rb_find(root, node->key);
		if(!entry)
			return rb_insert(root, node, false);

		gp = rb_grandparent(entry);
		p = entry->parent;

		if(rb_acquire_area_rr(gp, p, entry))
			done = true;
	} while(!done);

	if(entry != node)
		rb_lpush(entry, node);

	rb_release_area_rr(gp, p, entry);
	return entry;
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
		rb_set_root(root, left);
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
		rb_set_root(root, right);
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

static struct rb_node *rb_find_insert(struct rb_root *root,
		struct rb_node *node)
{
	struct rb_node *tree,
		      *tmp = NULL,
		      *rn;

	tree = rb_get_root(root);
	for(;;) {
		rn = rb_get_root(root);

		if(!tree) {
			return (tmp == rn) ? 
				rn : tmp;
		}

		tmp = tree;
		rb_lock_node(tmp);
		if(node->key <= tree->key)
			tree = tree->left;
		else
			tree = tree->right;
		rb_unlock_node(tmp);
	}

	return node;
}

typedef enum {
	INSERT_SUCCESS,
	INSERT_RETRY,
} rb_insert_t;

static void rb_balance(struct rb_root *root, struct rb_node *node);
static struct rb_node *rb_balance_rr(struct rb_root *root,
					struct rb_node      *node);
static void rb_fixup_bt(struct rb_root *root, struct rb_node *node)
{
	struct rb_node *rn;

	rn = rb_get_root(root);
	while(node && node != rn && rb_red(node) && !rb_unlinked(node)) {
		node = rb_balance_rr(root, node);
		rn = rb_get_root(root);
	
		if(node && (!rb_red(node->left) && !rb_red(node->right))) {
			break;
		}
	}

	rb_set_blk(rn);
}

static inline struct rb_node *rb_push_red(struct rb_node *p, struct rb_node *n)
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

static struct rb_node *raw_rb_balance_rr(struct rb_root *root,
					    struct rb_node *node)
{
	struct rb_node *p,
		      *s,
		      *tmp = NULL,
		      *orig = node;
	bool extra_lock = false;

	p = node->parent;
	s = rb_sibling(node);

	if(rb_red(p) || !rb_red(node))
		return node;
	else if(!rb_red(node->left) && !rb_red(node->right))
		return node;

	if(s && test_and_clear_bit(RB_NODE_RED_FLAG, &s->flags))
		return rb_push_red(p, node);

	if(p->left == node && rb_red(node->right)) {
		/* rotate left */
		tmp = node->right;
		rb_lock_node(tmp);
		rb_rotate_left(root, node);
		node = node->parent;
		extra_lock = true;
	} else if(p->right == node && rb_red(node->left)) {
		/* rotate right */
		tmp = node->left;
		rb_lock_node(tmp);
		rb_rotate_right(root, node);
		node = node->parent;
		extra_lock = true;
	}

	p = node->parent;
	if(p->left == node) {
		if(!tmp)
			tmp = node->left;
		rb_rotate_right(root, p);
	} else {
		if(!tmp)
			tmp = node->right;
		rb_rotate_left(root, p);
	}

	if(test_and_clear_bit(RB_NODE_DBLK_FLAG, &p->flags)) {
		rb_set_blk(p);
		rb_set_blk(orig);
		rb_set_blk(tmp);
	} else {
		/* swap colors between n and pre-rotation parent */
		rb_swap_color(node, p);
	}

	if(extra_lock)
		rb_unlock_node(tmp);
	return node;
}

static inline struct rb_node *rb_acquire_area_rr(struct rb_node *gp,
					     struct rb_node *p,
					     struct rb_node *n)
{
	rb_lock_node(gp);
	if(gp != rb_grandparent(n) || rb_unlinked(gp)) {
		rb_unlock_node(gp);
		return NULL;
	}

	rb_lock_node(p);
	if(p != n->parent || rb_unlinked(p)) {
		rb_unlock_node(p);
		rb_unlock_node(gp);
		return NULL;
	}

	rb_lock_node(n);
	if(rb_unlinked(n)) {
		rb_unlock_node(n);
		rb_unlock_node(p);
		rb_unlock_node(gp);
		return NULL;
	}
	return n;
}

static inline void rb_release_area_rr(struct rb_node *gp, struct rb_node *p,
				   struct rb_node *n)
{
	rb_unlock_node(n);
	rb_unlock_node(p);
	rb_unlock_node(gp);
}

static struct rb_node *rb_balance_rr(struct rb_root *root,
					struct rb_node      *node)
{
	struct rb_node *rv = node;
	struct rb_node *gp = rb_grandparent(node),
		      *p = node->parent,
		      *rn = rb_get_root(root);

	if((p && test_bit(RB_NODE_RED_FLAG, &p->flags)) && 
			test_bit(RB_NODE_RED_FLAG, &node->flags)) {
		rb_balance(root, p);
		return node;
	}

	if(!rb_acquire_area_rr(gp, p, node))
		return node;

	if(rn == rb_get_root(root))
		rv = raw_rb_balance_rr(root, node);

	rb_release_area_rr(gp, p, node);

	return rv;
}

static rb_insert_t rb_attempt_insert(struct rb_root *root,
					     struct rb_node **_node,
					     struct rb_node *new)
{
	struct rb_node *node = *_node,
		      *parent = node->parent,
		      *gp = rb_grandparent(node),
		      *rn = rb_get_root(root);

	rb_insert_t rv = INSERT_RETRY;

	if(!rb_acquire_area_rr(gp, parent, node))
		return rv;

	if(rn == rb_get_root(root)) {
		if(new->key <= node->key && !node->left) {
			node->left = new;
			new->parent = node;
			*_node = raw_rb_balance_rr(root, node);
			rv = INSERT_SUCCESS;
		} else if(new->key > node->key && !node->right) {
			node->right = new;
			new->parent = node;
			*_node = raw_rb_balance_rr(root, node);
			rv = INSERT_SUCCESS;
		}
	}

	rb_release_area_rr(gp, parent, node);

	return rv;
}

static void __rb_insert(struct rb_root *root, struct rb_node *new)
{
	struct rb_node *node;
	rb_insert_t update = INSERT_RETRY;
	
	do {
		node = rb_find_insert(root, new);

		if(!node) {
			rb_lock_root(root);
			if(!root->tree) {
				root->tree = new;
				rb_set_blk(new);
				rb_unlock_root(root);
				return;
			} else {
				update = INSERT_RETRY;
				rb_unlock_root(root);
				continue;
			}
			rb_unlock_root(root);
		}
		update = rb_attempt_insert(root, &node, new);
	} while(update == INSERT_RETRY);

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

struct rb_node *rb_find_leftmost(struct rb_node *tree)
{
	struct rb_node *tmp;

	if(!tree)
		return NULL;

	tmp = tree;
	for(;;) {
		rb_lock_node(tmp);
		if(!tree->left) {
			rb_unlock_node(tmp);
			return tree;
		}

		tmp = tree;
		tree = tree->left;
		rb_unlock_node(tmp);
	}
}

struct rb_node *rb_find_rightmost(struct rb_node *tree)
{
	struct rb_node *tmp;

	if(!tree)
		return NULL;

	tmp = tree;
	for(;;) {
		rb_lock_node(tmp);
		if(!tree->right) {
			rb_unlock_node(tmp);
			return tree;
		}

		tmp = tree;
		tree = tree->right;
		rb_unlock_node(tmp);
	}
}

static struct rb_node *rb_find_replacement(struct rb_node *node)
{
	struct rb_node *rv;

	rv = rb_find_leftmost(node);

	if(!rv) {
		return rb_find_leftmost(node->right);
	}
	
	return rv->right ? rv->right : rv;
}

static void rb_replace_node(struct rb_root *root,
				struct rb_node      *orig,
				struct rb_node      *replacement)
{
	replacement->left = orig->left;
	replacement->right = orig->right;
	replacement->parent = orig->parent;

	replacement->next = orig->next;
	replacement->prev = orig->prev;

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

	atomic_flags_copy(&replacement->flags, &orig->flags);
}

static void __rb_iterate(struct rb_node *node,
			     void (*fn)(struct rb_node *))
{
	if(!node)
		return;

	fn(node);
	__rb_iterate(node->left, fn);
	__rb_iterate(node->right, fn);
}

void rb_iterate(struct rb_root *root, void (*fn)(struct rb_node *))
{
	if(!root->tree)
		return;

	__rb_iterate(root->tree->left, fn);
	__rb_iterate(root->tree->right, fn);

	fn(root->tree);
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

static struct rb_node *raw_rb_resolve_nephew(struct rb_root *root,
					    struct rb_node *p,
					    struct rb_node *n,
					    struct rb_node *s,
					    struct rb_node *fn,
					    struct rb_node *cn)
{
	assert(p != NULL);
	assert(s != NULL);

	if(rb_dblk(p))
		return n;

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

	if(rb_dblk(n)) {
		rb_set_blk(n);
	} else {
		rb_set_red(n);
	}

	return RB_BALANCED;
}

static struct rb_node *rb_resolve_nephew(struct rb_root *root,
					struct rb_node *p,
					struct rb_node *n, struct rb_node *s,
					struct rb_node *fn, bool ndir)
{
	struct rb_node *tmp = NULL, *cn, *rv;

	assert(p != NULL);
	assert(s != NULL);
	
	if(fn == s->right)
		cn = s->left;
	else
		cn = s->right;

	if(rb_red(cn) && rb_blk(fn))
		tmp = ndir ? s->left : s->right;

	if(tmp)
		rb_lock_node(tmp);

	rv = raw_rb_resolve_nephew(root, p, n, s, fn, cn);

	if(tmp)
		rb_unlock_node(tmp);

	return rv;
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

	if(rb_dblk(p))
		return n;

	rb_lock_node(ns);
	if(rb_red(ns) || rb_red(p)) {
		goto err_l;
	} else {
		/* p = black */
		rb_set_blk(s);
		rb_set_red(p);
	}

	if(ndir)
		rb_rotate_left(root, p);
	else
		rb_rotate_right(root, p);

	n = rb_resolve_black_node(root, p, n, ns, ndir);
err_l:
	rb_unlock_node(ns);
	return n;
}

static inline void rb_release_area_bb(struct rb_node *gp,
				      struct rb_node *p,
				      struct rb_node *n,
				      struct rb_node *s)
{
	rb_unlock_node(s);
	rb_release_area_rr(gp, p, n);
}

static inline bool rb_acquire_area_bb(struct rb_node *gp,
				      struct rb_node *p,
				      struct rb_node *n,
				      struct rb_node *s)
{
	if(!rb_acquire_area_rr(gp, p, n))
		return false;

	rb_lock_node(s);
	if(rb_unlinked(s)) {
		rb_unlock_node(s);
		rb_release_area_rr(gp, p, n);
		return false;
	}

	return true;
}

static bool __rb_remove_duplicate(struct rb_root *root,
				      struct rb_node      *node,
				      const void *arg)
{
	struct rb_node *replacement, *tmp,
		      *gp, *p, *s;
	struct rb_node *nxt, *prev;

	gp = rb_grandparent(node);
	p = node->parent;
	s = rb_sibling(node);

	if(!rb_acquire_area_bb(gp, p, node, s))
		return false;

	if(!rb_node_has_duplicates(node)) {
		clear_bit(RB_NODE_HAS_DUPLICATES_FLAG, &node->flags);
		rb_release_area_bb(gp, p, node, s);
		return false;
	}

	tmp = rb_find_duplicate(root, node->key, arg);

	if(test_bit(RB_NODE_ACQUIRED_FLAG, &tmp->flags)) {
		rb_release_area_bb(gp, p, node, s);
		return false;
	}
	
	if(tmp == node) {
		replacement = node->next;
		rb_lock_node(replacement);

		if(!replacement) {
			clear_bit(RB_NODE_HAS_DUPLICATES_FLAG, &node->flags);
			rb_unlock_node(replacement);
			rb_release_area_bb(gp, p, node, s);
			return false;
		}
		nxt = replacement->next;
		prev = NULL;

		set_bit(RB_NODE_UNLINKED_FLAG, &node->flags);
		rb_replace_node(root, node, replacement);
		replacement->next = nxt;
		replacement->prev = prev;

		node->left = node->right = node->parent = NULL;
		clear_bit(RB_NODE_UNLINKED_FLAG, &replacement->flags);
		rb_unlock_node(node);
		node = replacement;
	} else {
		rb_pop(tmp);
	}

	if(rb_node_has_duplicates(node))
		set_bit(RB_NODE_HAS_DUPLICATES_FLAG, &node->flags);
	else
		clear_bit(RB_NODE_HAS_DUPLICATES_FLAG, &node->flags);

	rb_release_area_bb(gp, p, node, s);
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

static void rb_fixup_bb(struct rb_root *root, struct rb_node *node);
static struct rb_node *rb_balance_bb(struct rb_root *root,
				    struct rb_node *node)
{
	struct rb_node *gp, *p, *s;
	bool ndir;
	struct rb_node *rv;

	gp = rb_grandparent(node);
	p = node->parent;
	s = rb_sibling(node);

	assert(p != NULL);
	assert(s != NULL);

	if(!test_bit(RB_NODE_REMOVE_FLAG, &node->flags) && !rb_dblk(node))
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

	if(!rb_acquire_area_bb(gp, p, node, s))
		return node;

	rv = raw_rb_balance_bb(root, p, node, s);

	rb_release_area_bb(gp, p, node, s);

	return rv;
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
		rb_fixup_bt(root, node);
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

	rb_lock_root(root);
	rn = root->tree;
	if(node == rn && !node->left && !node->right) {
		rb_remove_node(NULL, node);
		root->tree = NULL;

		*imbalance = RB_BALANCED;
		return true;
	}
	rb_unlock_root(root);

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
			rb_lock_root(root);
			/* current is root */
			root->tree = (current->left) ? 
					current->left : current->right;
			clear_bit(RB_NODE_RED_FLAG, &root->tree->flags);
			root->tree->parent = NULL;
			rb_unlock_root(root);
		}

		action = REMOVE_TERMINATE;
		set_bit(RB_NODE_UNLINKED_FLAG, &current->flags);
		break;

	case BLACK_NO_CHILDREN:
		rb_lock_root(root);
		if(root->tree == current) {
			root->tree = NULL;
			action = REMOVE_TERMINATE;
			rb_unlock_root(root);
			break;
		}
		rb_unlock_root(root);

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
	struct rb_node *gp, *p, *s,
		      *replacement = NULL,
		      *balance = RB_BALANCED;
	rb_delete_t action;

	gp = rb_grandparent(node);
	p = node->parent;
	s = rb_sibling(node);

	if(!rb_acquire_area_bb(gp, p, node, s))
		return NULL;

	if(rb_dblk(node) || rb_unlinked(node) ||
			test_bit(RB_NODE_ACQUIRED_FLAG, &node->flags)) {
		rb_release_area_bb(gp, p, node, s);
		return NULL;
	}

	if(orig) {
		if(node != rb_find_replacement(orig) || (node->left &&
					node->right)) {
			rb_release_area_bb(gp, p, node, s);
			return NULL;
		}

	}

	if(node->left && node->right) {
		replacement = rb_find_replacement(node);
		if(!replacement) {
			rb_release_area_bb(gp, p, node, s);
			return NULL;
		}

		if(raw_rb_remove(root, replacement, node) != replacement) {
			rb_release_area_bb(gp, p, node, s);
			return NULL;
		} else {
			rb_lock_node(replacement);
			rb_replace_node(root, node, replacement);
			set_bit(RB_NODE_UNLINKED_FLAG, &node->flags);
			rb_unlock_node(node);
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

struct rb_node *rb_remove(struct rb_root *root,
			     u64 key,
			     const void *arg)
{
	struct rb_node *find;
	bool done = false;

	do {
		find = rb_find(root, key);
		if(!find) {
			return NULL;
		}

		if(rb_dblk(find) || rb_unlinked(find))
			continue;

		if(test_bit(RB_NODE_HAS_DUPLICATES_FLAG, &find->flags)) {
			done =__rb_remove_duplicate(root, find, arg);
		} else {
			if(raw_rb_remove(root, find, NULL)) {
				atomic64_dec(root->num);
				done = true;
			}
		}
	} while(!done);

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

void rb_dump(struct rb_root *root, FILE *stream)
{
	rb_dump_node(root->tree, stream);
	fputc('\n', stream);
}

