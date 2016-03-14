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
#include <xfiredb/object.h>
#include <xfiredb/rbtree.h>
#include <xfiredb/hashmap.h>
#include <xfiredb/mem.h>
#include <xfiredb/error.h>

/**
 * @brief Allocate a new iterator.
 * @param map Hashmap to allocate a new iterator for.
 * @return The allocated iterator, or \p NULL in case of error.
 */
struct hashmap_iterator *hashmap_new_iterator(struct hashmap *map)
{
	struct hashmap_iterator *it;

	it = xfiredb_zalloc(sizeof(*it));
	it->it = skiplist_iterator_new(&map->list);
	return it;
}

/**
 * @brief Get the next node from an iterator.
 * @param it Iterator to move to the next node.
 * @return The next node, or \p NULL if the iterator is at the end.
 */
struct hashmap_node *hashmap_iterator_next(struct hashmap_iterator *it)
{
	struct skiplist_node *node;

	if(!it || !it->it)
		return NULL;

	node = skiplist_iterator_next(it->it);

	if(!node)
		return NULL;

	return container_of(node, struct hashmap_node, node);
}

struct hashmap_node *hashmap_iterator_delete(struct hashmap_iterator *it)
{
	struct skiplist_node *node;

	node = skiplist_iterator_delete(it->it);
	return container_of(node, struct hashmap_node, node);
}

/**
 * @brief Free an iterator.
 * @param it Iterator to free.
 */
void hashmap_free_iterator(struct hashmap_iterator *it)
{
	skiplist_iterator_free(it->it);
	xfiredb_free(it);
}

/**
 * @brief Initialise a hashmap.
 * @param hm Hashmap to initialise.
 */
void hashmap_init(struct hashmap *hm)
{
	skiplist_init(&hm->list);
	atomic_init(&hm->num);
}

/**
 * @brief Destroy a hashmap node.
 * @param n Node to destroy.
 */
void hashmap_node_destroy(struct hashmap_node *n)
{
	xfiredb_free(n->key);
	skiplist_node_destroy(&n->node);
}

/**
 * @brief Add a hashmap node.
 * @param hm Hashmap to add to.
 * @param key Key to add \p n under.
 * @param n Node to add under \p key.
 */
int hashmap_add(struct hashmap *hm, char *key, struct hashmap_node *n)
{
	char *_key;

	xfiredb_sprintf(&_key, "%s", key);
	n->key = _key;
	if(skiplist_insert(&hm->list, key, &n->node) == -XFIREDB_OK) {
		atomic_inc(hm->num);
		return -XFIREDB_OK;
	}

	return -XFIREDB_ERR;
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
	struct skiplist_node *node;

	node = skiplist_search(&hm->list, key);
	if(skiplist_delete(&hm->list, key) == -XFIREDB_OK)
		atomic_dec(hm->num);
	else
		return NULL;

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
	struct skiplist_node *node;

	node = skiplist_search(&hm->list, key);
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
	skiplist_destroy(&hm->list);
	atomic_destroy(&hm->num);
}

/** @} */

