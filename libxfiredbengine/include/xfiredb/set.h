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

#ifndef __XFIREDB_SET_H__
#define __XFIREDB_SET_H__

#include <stdlib.h>
#include <stdio.h>

#include <xfiredb/xfiredb.h>
#include <xfiredb/types.h>
#include <xfiredb/mem.h>
#include <xfiredb/rbtree.h>
#include <xfiredb/error.h>

/**
 * @brief Set datastructure.
 */
struct set {
	struct rb_root root; //!< Red-black tree root.
	atomic_t num; //!< Set size.
};

/**
 * @brief Set key structure.
 */
struct set_key {
	char *key; //!< Key of the set-key.
	struct rb_node node; //!< Red-black tree entry.
};

/**
 * @brief Set iterator structure.
 */
struct set_iterator {
	struct rb_iterator *it; //!< Red-black tree iterator.
};

/**
 * @brief Iterate over a set.
 * @param __s Set to iterate over.
 * @param __k Key carriage.
 * @param __it Set iterator.
 */
#define for_each_set(__s, __k, __it) \
	for(__k = set_iterator_next(__it); __k; \
			__k = set_iterator_next(__it))

CDECL
extern void set_init(struct set *s);
extern void set_key_init(struct set_key *key, const char *k);
extern struct set_key *set_key_alloc(const char *k);
extern void set_destroy(struct set *s);
extern void set_key_destroy(struct set_key *k);
extern struct set_iterator *set_iterator_new(struct set *s);
extern void set_iterator_free(struct set_iterator *si);
extern struct set_key *set_iterator_next(struct set_iterator *it);
extern int set_add(struct set *s, char *key, struct set_key *k);
extern bool set_contains(struct set *s, const char *key);
extern struct set_key *set_remove(struct set *s, const char *key);
extern int set_clear(struct set *set);

/**
 * @brief Get the number of keys in a set.
 * @param set Set to get the size of.
 * @return The number of elements is \p set.
 */
static inline int set_size(struct set *set)
{
	return atomic_get(&set->num);
}
CDECL_END

#endif

/** @} */

