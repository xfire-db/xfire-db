/*
 *  Skiplist
 *  Copyright (C) 2016   Michel Megens <dev@michelmegens.net>
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
 * @addtogroup skiplist
 * @{
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include <xfiredb/xfiredb.h>
#include <xfiredb/types.h>
#include <xfiredb/error.h>
#include <xfiredb/mem.h>
#include <xfiredb/object.h>
#include <xfiredb/skiplist.h>

void skiplist_init(struct skiplist *l)
{
	raw_skiplist_init(l, SKIPLIST_DEFAULT_PROB);
}

void raw_skiplist_init(struct skiplist *l, double p)
{
	struct skiplist_node *node;
	int i = 0;

	if(!l)
		return;

	node = xfiredb_zalloc(sizeof(*node));
	node->key = NULL;
	node->hash = SKIPLIST_MAX_SIZE;
	node->forward = xfiredb_zalloc(sizeof(node) * (SKIPLIST_MAX_LEVELS + 1));

	for(i = 0; i <= SKIPLIST_MAX_LEVELS; i++)
		node->forward[i] = node;

	l->header = node;
	l->prob = p;
	l->level = 1;
	l->size = 0UL;
}

struct skiplist *skiplist_alloc(void)
{
	return raw_skiplist_alloc(SKIPLIST_DEFAULT_PROB);
}

struct skiplist *raw_skiplist_alloc(double prob)
{
	struct skiplist *l;

	if(prob > 1.0f)
		return NULL;

	l = xfiredb_zalloc(sizeof(*l));
	raw_skiplist_init(l, prob);

	return l;
}

void skiplist_destroy(struct skiplist *l)
{
	if(!l || !l->header || !l->header->forward)
		return;

	xfiredb_free(l->header->forward);
	xfiredb_free(l->header);
}

void skiplist_free(struct skiplist *l)
{
	if(!l)
		return;

	skiplist_destroy(l);
	xfiredb_free(l);
}

static inline double skiplist_rand(void)
{
	return ((double)rand()) / ((double)RAND_MAX);
}

static int skiplist_rand_level(struct skiplist *l, int max)
{
	int height = 0;

	do {
		height++;
	} while(height < max && skiplist_rand() < l->prob);

	return height;
}

#if 1
#define SKIPLIST_SEED 0x8FE3C9A1
#define MURMUR_C1 0xcc9e2d51
#define MURMUR_C2 0x1b873593
#define MURMUR_R1 15
#define MURMUR_R2 13
#define MURMUR_MIX1 5
#define MURMUR_MIX2 0xe6546b64

/**
 * @brief Hash a skiplist key.
 * @param key Key to be hashed.
 * @param seed Hashing seed.
 * @note The seed should be set to SKIPLIST_SEED in every
 *       case.
 *
 * This is an implementation if the murmur version 3 hash. See
 * https://gowalker.org/github.com/spaolacci/murmur3 for benchmark
 * results.
 */
static u32 skiplist_hash_key(const char *key, u32 seed)
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
#else

#define SKIPLIST_SEED 2166136261
static u32 skiplist_hash_key(const char *key, u32 seed)
{
	int i, len;
	u32 hash = seed;

	len = strlen(key);
	for(i = 0; i < len; i++) {
		hash ^= key[i];
		hash *= 16777619;
	}

	return hash;
}
#endif

static inline void skiplist_set_key(struct skiplist_node *node, const char *key)
{
	int len;

	len = strlen(key);
	node->key = xfiredb_zalloc(len + 1);
	memcpy(node->key, key, len);
}

struct skiplist_node *skiplist_search(struct skiplist *l, const char *key)
{
	int i;
	u32 hash;
	struct skiplist_node *node;

	hash = skiplist_hash_key(key, SKIPLIST_SEED);
	node = l->header;
	for(i = l->level; i >= 1; i--) {
		while(node->forward[i]->hash < hash)
			node = node->forward[i];
	}

	if(node->forward[1]->hash == hash) {
		if(strcmp(node->forward[1]->key, key)) {
			while(strcmp(node->forward[1]->key, key))
					node = node->forward[1];
		}

		if(node->forward[1]->hash == hash)
			return node->forward[1];
		else
			return NULL;
	}

	return NULL;
}

int skiplist_insert(struct skiplist *list, const char *key, struct skiplist_node *node)
{
	struct skiplist_node *update[SKIPLIST_MAX_LEVELS + 1];
	struct skiplist_node *carriage;
	u32 hash;
	int i, level;

	hash = skiplist_hash_key(key, SKIPLIST_SEED);
	carriage = list->header;
	for(i = list->level; i >= 1; i--) {
		while(carriage->forward[i]->hash < hash)
			carriage = carriage->forward[i];

		update[i] = carriage;
	}

	carriage = carriage->forward[1];

	if(carriage->key && !strcmp(carriage->key, key)) {
		return -XFIREDB_OK;
	} else {
		level = skiplist_rand_level(list, SKIPLIST_MAX_LEVELS);

		if(level > list->level) {
			for(i = list->level + 1; i <= level; i++) {
				update[i] = list->header;
			}

			list->level = level;
		}

		node->hash = hash;
		skiplist_set_key(node, key);
		node->forward = xfiredb_zalloc(sizeof(node) * (level+1));

		for(i = 1; i <= level; i++) {
			node->forward[i] = update[i]->forward[i];
			update[i]->forward[i] = node;
		}
	}

	return -XFIREDB_OK;
}

void skiplist_node_destroy(struct skiplist_node *node)
{
	if(!node)
		return;

	xfiredb_free(node->forward);
	xfiredb_free(node->key);
}

int skiplist_delete(struct skiplist *list, const char *key)
{
	int i;
	u32 hash;
	struct skiplist_node *update[SKIPLIST_MAX_LEVELS + 1];
	struct skiplist_node *x = list->header;

	hash = skiplist_hash_key(key, SKIPLIST_SEED);
	i = list->level;

	for(; i >= 1; i--) {
		while(x->forward[i]->hash < hash) {
			x = x->forward[i];
		}

		update[i] = x;
	}

	if(x->forward[1]->hash == hash) {
		x = x->forward[1];

		for(i = 1; i <= list->level; i++) {
			if(update[i]->forward[i] != x)
				break;
			update[i]->forward[i] = x->forward[i];
		}


		skiplist_node_destroy(x);
		while(list->level > 1 && list->header->forward[list->level]
				== list->header)
			list->level--;

		return -XFIREDB_OK;
	}

	return -XFIREDB_ERR;
}

struct skiplist_iterator *skiplist_iterator_new(struct skiplist *l)
{
	struct skiplist_iterator *it;

	it = xfiredb_zalloc(sizeof(*it));
	it->list = l;
	it->prev = l->header;
	it->current = l->header;

	return it;
}

struct skiplist_node *skiplist_iterator_next(struct skiplist_iterator *it)
{
	it->prev = it->current;
	it->current = it->current->forward[1];
	
	if(it->current == it->list->header)
		it->current = NULL;

	return it->current;
}

struct skiplist_node *skiplist_iterator_delete(struct skiplist_iterator *it)
{
	struct skiplist *l = it->list;

	if(skiplist_delete(l, it->current->key) != -XFIREDB_OK)
		return it->current;

	it->current = it->prev;
	return it->current;
}

void skiplist_iterator_free(struct skiplist_iterator *it)
{
	if(!it)
		return;

	xfiredb_free(it);
}

#ifdef HAVE_DEBUG
void skiplist_dump(FILE *stream, struct skiplist *l)
{
	struct skiplist_node *x = l->header;

	while(x && x->forward[1] != l->header) {
		fprintf(stream, "%u[%s]->", x->forward[1]->hash, x->forward[1]->key);
		x = x->forward[1];
	}

	fprintf(stream, "0[nil]\n");
}
#endif

/** @} */

