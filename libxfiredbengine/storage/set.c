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

/**
 * @addtogroup set
 * @{
 */

#include <stdlib.h>
#include <stdio.h>

#include <xfiredb/xfiredb.h>
#include <xfiredb/object.h>
#include <xfiredb/types.h>
#include <xfiredb/mem.h>
#include <xfiredb/error.h>
#include <xfiredb/set.h>
#include <xfiredb/skiplist.h>

/**
 * @brief Initialise a new set.
 * @param s Set to initialise.
 */
void set_init(struct set *s)
{
	skiplist_init(&s->list);
	object_init(&s->obj);
	atomic_init(&s->num);
}

/**
 * @brief Initialise a new set key.
 * @param key Key to initialise.
 * @param k String representation of the key.
 */
void set_key_init(struct set_key *key, const char *k)
{
	int len = strlen(k);

	key->key = xfiredb_zalloc(len + 1);
	memcpy((char*)key->key, k, len);
}

/**
 * @brief Allocate a new set key.
 * @param k String representation of the set key.
 * @return The new set key.
 */
struct set_key *set_key_alloc(const char *k)
{
	struct set_key *key = xfiredb_zalloc(sizeof(*k));

	set_key_init(key, k);
	return key;
}

/**
 * @brief Destroy a set.
 * @param s Set to destroy.
 */
void set_destroy(struct set *s)
{
	skiplist_destroy(&s->list);
	atomic_destroy(&s->num);
}

/**
 * @brief Destroy a set key.
 * @param k Set key to destroy.
 */
void set_key_destroy(struct set_key *k)
{
	xfiredb_free(k->key);
	skiplist_node_destroy(&k->node);
}

/**
 * @brief Allocate a new iterator.
 * @param s Set allocate an iterator for.
 * @return An iterator for \p s.
 */
struct set_iterator *set_iterator_new(struct set *s)
{
	struct set_iterator *si = xfiredb_zalloc(sizeof(*si));

	si->it = skiplist_iterator_new(&s->list);
	return si;
}

/**
 * @brief Free a previously allocated iterator.
 * @param si Set iterator to free.
 */
void set_iterator_free(struct set_iterator *si)
{
	skiplist_iterator_free(si->it);
	xfiredb_free(si);
}

/**
 * @brief Get the next element during iteration from an iterator.
 * @param it Iterator.
 * @return The next element (set key).
 */
struct set_key *set_iterator_next(struct set_iterator *it)
{
	struct skiplist_node *node;

	node = skiplist_iterator_next(it->it);

	if(!node)
		return NULL;

	return container_of(node, struct set_key, node);
}

/**
 * @brief Add a new key to a set.
 * @param s Set to add to.
 * @param key Key to add.
 * @param k Set key to add to \p s.
 * @return An error code.
 */
int set_add(struct set *s, char *key, struct set_key *k)
{
	if(set_contains(s, key))
		return -XFIREDB_ERR;

	xfiredb_sprintf(&k->key, "%s", key);
	if(skiplist_insert(&s->list, key, &k->node) == -XFIREDB_OK) {
		atomic_inc(s->num);
		return -XFIREDB_OK;
	}

	return -XFIREDB_ERR;
}

/**
 * @brief Check if a set contains a key.
 * @param s Set to check.
 * @param key Key to search for.
 * @return True if \p s contains \p key, false otherwise.
 */
bool set_contains(struct set *s, const char *key)
{
	return skiplist_search(&s->list, key) ? true : false;
}

/**
 * @brief Remove a given key from a given set.
 * @param s Set to remove from.
 * @param key Key to remove from \p s.
 * @return The removed set key. NULL if no key was removed.
 */
struct set_key *set_remove(struct set *s, const char *key)
{
	struct skiplist_node *node;

	node = skiplist_search(&s->list, key);
	if(skiplist_delete(&s->list, key) == -XFIREDB_OK) {
		atomic_dec(s->num);
		return container_of(node, struct set_key, node);
	}

	return NULL;
}

/**
 * @brief Remove all keys from a set.
 * @param set Set to clear.
 * @return An error code.
 */
int set_clear(struct set *set)
{
	struct set_key *k;
	struct skiplist_iterator *it;
	struct skiplist_node *node;

	it = skiplist_iterator_new(&set->list);
	while((node = skiplist_iterator_next(it)) != NULL) {
		node = skiplist_iterator_to_node(it);
		k = container_of(node, struct set_key, node);
		skiplist_iterator_delete(it);
		xfiredb_free(k->key);
		xfiredb_free(k);
	}
	skiplist_iterator_free(it);

	return -XFIREDB_OK;
}

/** @} */

