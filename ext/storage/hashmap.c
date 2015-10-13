/*
 *  Hashmap header
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
#include <time.h>

#include <xfire/xfire.h>
#include <xfire/types.h>
#include <xfire/string.h>
#include <xfire/rbtree.h>
#include <xfire/hashmap.h>
#include <xfire/mem.h>
#include <xfire/error.h>

#define HM_SEED 0x8FE3C9A1
#define MURMUR_C1 0xcc9e2d51
#define MURMUR_C2 0x1b873593
#define MURMUR_R1 15
#define MURMUR_R2 13
#define MURMUR_MIX1 5
#define MURMUR_MIX2 0xe6546b64

/**
 * @brief Hash a dictionary key.
 * @param key Key to be hashed.
 * @param seed Hashing seed.
 * @note The seed should be set to HM_SEED in every
 *       case.
 *
 * This is an implementation if the murmur version 3 hash. See
 * https://gowalker.org/github.com/spaolacci/murmur3 for benchmark
 * results.
 */
static u32 hashmap_hash(const char *key, u32 seed)
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

static bool hashmap_cmp_node(struct rb_node *node, const void *arg)
{
	const char *key = arg;
	struct hashmap_node *n = container_of(node, struct hashmap_node, node);

	if(!strcmp(n->key, key))
		return true;

	return false;
}

void hashmap_init(struct hashmap *hm)
{
	rb_init_root(&hm->root);
	hm->root.cmp = hashmap_cmp_node;
}

int hashmap_add(struct hashmap *hm, char *_key, char *value)
{
	u32 key;
	struct hashmap_node *node;

	key = hashmap_hash(_key, HM_SEED);
	node = xfire_zalloc(sizeof(*node));
	rb_init_node(&node->node);	
	string_init(&node->s);
	string_set(&node->s, value);
	xfire_sprintf(&node->key, "%s", _key);

	node->node.key = key;
	return rb_insert(&hm->root, &node->node, true) ? -XFIRE_OK : -XFIRE_ERR;
}

int hashmap_remove(struct hashmap *hm, char *key)
{
	struct rb_node *node;
	struct hashmap_node *n;
	u32 hash = hashmap_hash(key, HM_SEED);

	node = rb_remove(&hm->root, hash, key);
	if(!node)
		return -XFIRE_ERR;

	n = container_of(node, struct hashmap_node, node);
	string_destroy(&n->s);
	rb_node_destroy(&n->node);
	xfire_free(n->key);
	xfire_free(n);

	return -XFIRE_OK;
}

int hashmap_find(struct hashmap *hm, char *key, char **buff)
{
	struct rb_node *node;
	struct hashmap_node *n;
	u32 hash = hashmap_hash(key, HM_SEED);

	node = rb_find_duplicate(&hm->root, hash, key);
	if(!node)
		return -XFIRE_ERR;

	n = container_of(node, struct hashmap_node, node);
	string_get(&n->s, buff);
	return -XFIRE_OK;
}

static void hashmap_destroy_hook(struct rb_root *root, struct rb_node *n, void *arg)
{
	struct hashmap_node *node;
	struct hashmap *map;

	node = container_of(n, struct hashmap_node, node);
	map = container_of(root, struct hashmap, root);
	hashmap_remove(map, node->key);
}

static void hashmap_iterate_hook(struct rb_root *root, struct rb_node *node, void *arg)
{
	struct hashmap *map = container_of(root, struct hashmap, root);
	struct hashmap_node *n = container_of(node, struct hashmap_node, node);
	void (*fn)(struct hashmap *map, struct hashmap_node *n) = arg;

	fn(map, n);
}

void hashmap_iterate(struct hashmap *map, void (*fn)(struct hashmap *hm, struct hashmap_node *n))
{
	rb_iterate(&map->root, hashmap_iterate_hook, fn);
}

void hashmap_destroy(struct hashmap *hm)
{
	while(hm->root.tree)
		rb_iterate(&hm->root, &hashmap_destroy_hook, NULL);
	rb_destroy_root(&hm->root);
}

