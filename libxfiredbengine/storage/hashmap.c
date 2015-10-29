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

/**
 * @addtogroup hashmap
 * @{
 */

#include <stdlib.h>
#include <time.h>

#include <xfiredb/xfiredb.h>
#include <xfiredb/types.h>
#include <xfiredb/rbtree.h>
#include <xfiredb/hashmap.h>
#include <xfiredb/mem.h>
#include <xfiredb/error.h>

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

/**
 * @brief Initialise a hashmap.
 * @param hm Hashmap to initialise.
 */
void hashmap_init(struct hashmap *hm)
{
	rb_init_root(&hm->root);
	atomic_init(&hm->num);
	hm->root.cmp = hashmap_cmp_node;
}

/**
 * @brief Destroy a hashmap node.
 * @param n Node to destroy.
 */
void hashmap_node_destroy(struct hashmap_node *n)
{
	xfire_free(n->key);
	rb_node_destroy(&n->node);
}

/**
 * @brief Add a hashmap node.
 * @param hm Hashmap to add to.
 * @param key Key to add \p n under.
 * @param n Node to add under \p key.
 */
int hashmap_add(struct hashmap *hm, char *key, struct hashmap_node *n)
{
	u32 hash;
	char *_key;

	hash = hashmap_hash(key, HM_SEED);
	rb_init_node(&n->node);
	xfire_sprintf(&_key, "%s", key);
	n->node.key = hash;
	n->key = _key;
	if(rb_insert(&hm->root, &n->node, true)) {
		atomic_inc(hm->num);
		return -XFIRE_OK;
	}

	return -XFIRE_ERR;
}

/**
 * @brief Remove a hashmap node.
 * @param hm Hashmap to remove from.
 * @param key Key to remove.
 * @return The removed hashmap node, if any. If \p key was not 
 * found, NULL is returned.
 */
struct hashmap_node *hashmap_remove(struct hashmap *hm, char *key)
{
	struct rb_node *node;
	u32 hash = hashmap_hash(key, HM_SEED);

	node = rb_remove(&hm->root, hash, key);
	if(!node)
		return NULL;

	atomic_dec(hm->num);
	return container_of(node, struct hashmap_node, node);
}

/**
 * @brief Search for a key in a given hashmap.
 * @param hm Hashmap to search.
 * @param key Key to search for.
 * @return The hashmap node associated with \p key, or NULL.
 */
struct hashmap_node *hashmap_find(struct hashmap *hm, char *key)
{
	struct rb_node *node;
	u32 hash = hashmap_hash(key, HM_SEED);

	node = rb_find_duplicate(&hm->root, hash, key);
	if(!node)
		return NULL;

	return container_of(node, struct hashmap_node, node);
}

/**
 * @brief Destroy a hashmap.
 * @param hm Hashmap to destroy.
 */
void hashmap_destroy(struct hashmap *hm)
{
	rb_destroy_root(&hm->root);
	atomic_destroy(&hm->num);
}

/** @} */

