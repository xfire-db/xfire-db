/*
 *  Hashed sets
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

#include <xfiredb/xfiredb.h>
#include <xfiredb/types.h>
#include <xfiredb/mem.h>
#include <xfiredb/rbtree.h>
#include <xfiredb/error.h>
#include <xfiredb/set.h>

static bool set_cmp_node(struct rb_node *node, const void *arg)
{
	const char *key = arg;
	struct set_key *s = container_of(node, struct set_key, node);

	if(!strcmp(s->key, key))
		return true;

	return false;
}

void set_init(struct set *s)
{
	rb_init_root(&s->root);
	s->root.cmp = set_cmp_node;
	atomic_init(&s->num);
}

void set_key_init(struct set_key *key, const char *k)
{
	int len = strlen(k);

	key->key = xfire_zalloc(len + 1);
	memcpy((char*)key->key, k, len);

	rb_init_node(&key->node);
}

struct set_key *set_key_alloc(const char *k)
{
	struct set_key *key = xfire_zalloc(sizeof(*k));

	set_key_init(key, k);
	return key;
}

void set_destroy(struct set *s)
{
	rb_destroy_root(&s->root);
	atomic_destroy(&s->num);
}

void set_key_destroy(struct set_key *k)
{
	xfire_free(k->key);
	rb_node_destroy(&k->node);
}

struct set_iterator *set_iterator_new(struct set *s)
{
	struct set_iterator *si = xfire_zalloc(sizeof(*si));

	si->it = rb_new_iterator(&s->root);
	return si;
}

void set_iterator_free(struct set_iterator *si)
{
	rb_free_iterator(si->it);
	xfire_free(si);
}

struct set_key *set_iterator_next(struct set_iterator *it)
{
	struct rb_node *n;

	n = rb_iterator_next(it->it);

	if(!n)
		return NULL;

	return container_of(n, struct set_key, node);
}

#define SET_SEED 0x8FE3C9A1
#define MURMUR_C1 0xcc9e2d51
#define MURMUR_C2 0x1b873593
#define MURMUR_R1 15
#define MURMUR_R2 13
#define MURMUR_MIX1 5
#define MURMUR_MIX2 0xe6546b64

/**
 * @brief Hash a set key.
 * @param key Key to be hashed.
 * @param seed Hashing seed.
 * @note The seed should be set to SET_SEED
 *       case.
 *
 * This is an implementation if the murmur version 3 hash. See
 * https://gowalker.org/github.com/spaolacci/murmur3 for benchmark
 * results.
 */
static u32 set_hash_key(const char *key, u32 seed)
{
	u32 hash, nblocks, k1, k2, len;
	const u32 *blocks;
	const char *tail;
	int i;

	len = strlen(key);
	hash = seed;
	nblocks = len / 4;
	blocks = (u32*)key;
	k2 = 0;

	for (i = 0; i < nblocks; i++) {
		k1 = blocks[i];
		k1 *= MURMUR_C1;
		k1 = (k1 << MURMUR_R1) | (k1 >> (32 - MURMUR_R1));
		k1 *= MURMUR_C2;

		hash ^= k1;
		hash = ((hash << MURMUR_R2) | (hash >> 
				(32 - MURMUR_R2))) * MURMUR_MIX1 + MURMUR_MIX2;
	}

	tail = (const char*) (key + nblocks * 4);
	switch (len & 3) {
	case 3:
		k2 ^= tail[2] << 16;
	case 2:
		k2 ^= tail[1] << 8;
	case 1:
		k2 ^= tail[0];

		k2 *= MURMUR_C1;
		k2 = (k2 << MURMUR_R2) | (k2 >> (32 - MURMUR_R1));
		k2 *= MURMUR_C2;
		hash ^= k2;
	}

	hash ^= len;
	hash ^= (hash >> 16);
	hash *= 0x85ebca6b;
	hash ^= (hash >> 13);
	hash *= 0xc2b2ae35;
	hash ^= (hash >> 16);

	return hash;
}

int set_add(struct set *s, char *key, struct set_key *k)
{
	u32 hash;

	if(set_contains(s, key))
		return -XFIRE_ERR;

	rb_init_node(&k->node);
	xfire_sprintf(&k->key, "%s", key);
	hash = set_hash_key(k->key, SET_SEED);
	k->node.key = hash;

	if(rb_insert(&s->root, &k->node, true)) {
		atomic_inc(s->num);
		return -XFIRE_OK;
	}

	return -XFIRE_ERR;
}

bool set_contains(struct set *s, const char *key)
{
	u32 hash;
	struct rb_node *node;

	hash = set_hash_key(key, SET_SEED);
	node = rb_find_duplicate(&s->root, hash, key);

	return node ? true : false;
}

struct set_key *set_remove(struct set *s, const char *key)
{
	struct rb_node *node;
	u32 hash = set_hash_key(key, SET_SEED);

	node = rb_remove(&s->root, hash, key);
	if(!node)
		return NULL;

	atomic_dec(s->num);
	return container_of(node, struct set_key, node);
}

static inline struct set_key *set_clear_next(struct set *set)
{
	struct rb_node *node = rb_get_root(&set->root);
	struct set_key *key;

	if(!node)
		return NULL;

	key = container_of(node, struct set_key, node);
	return set_remove(set, key->key);
}

int set_clear(struct set *set)
{
	struct set_key *k;

	while((k = set_clear_next(set))) {
		xfire_free(k->key);
		xfire_free(k);
	}

	return -XFIRE_OK;
}
