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

#ifndef __HASHMAH_H__
#define __HASHMAH_H__

#include <stdlib.h>
#include <time.h>

#include <xfire/xfire.h>
#include <xfire/types.h>
#include <xfire/rbtree.h>

typedef struct rb_iterator* hashmap_iterator_t;

#define hashmap_clear_foreach(__hm, __n) for(__n = hashmap_clear_next(__hm); \
					     __n; __n = hashmap_clear_next(__hm))

/**
 * @brief Hashmap definition.
 */
struct hashmap {
	struct rb_root root; //!< Red-black tree root.
	atomic_t num; //!< Number of entry's in the hashmap.
	void *privdata; //!< Private data.
};

/**
 * @brief hashmap node.
 */
struct hashmap_node {
	struct rb_node node; //!< Red-black tree node.
	char *key; //!< Hashmap node key.
};

CDECL
/**
 * @brief Get the size of a hashmap.
 * @param map Map to get the size of.
 * @return The size of \p map.
 */
static inline s64 hashmap_size(struct hashmap *map)
{
	return atomic_get(&map->num);
}


extern struct hashmap_node *hashmap_remove(struct hashmap *hm, char *key);
extern int hashmap_add(struct hashmap *hm, char *key, struct hashmap_node *n);
extern void hashmap_init(struct hashmap *hm);
extern struct hashmap_node *hashmap_find(struct hashmap *hm, char *key);
extern void hashmap_destroy(struct hashmap *hm);
extern void hashmap_node_destroy(struct hashmap_node *n);

static inline hashmap_iterator_t hashmap_new_iterator(struct hashmap *map)
{
	return (hashmap_iterator_t)rb_new_iterator(&map->root);
}

static inline void hashmap_free_iterator(hashmap_iterator_t it)
{
	rb_free_iterator((struct rb_iterator*)it);
}

static inline struct hashmap_node *hashmap_iterator_next(hashmap_iterator_t it)
{
	struct rb_node *node = rb_iterator_next((struct rb_iterator*)it);

	if(!node)
		return NULL;

	return container_of(node, struct hashmap_node, node);
}

static inline struct hashmap_node *hashmap_clear_next(struct hashmap *map)
{
	struct rb_node *node = rb_get_root(&map->root);
	struct hashmap_node *hnode;

	if(!node)
		return NULL;

	hnode = container_of(node, struct hashmap_node, node);
	hashmap_remove(map, hnode->key);

	return hnode;
}
CDECL_END

#endif

/** @} */

