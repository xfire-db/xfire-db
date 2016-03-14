/*
 *  Skiplist header
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

#ifndef __SKIPLIST_H__
#define __SKIPLIST_H__

#include <stdlib.h>
#include <stdio.h>

#include <xfiredb/xfiredb.h>
#include <xfiredb/types.h>
#include <xfiredb/os.h>
#include <xfiredb/object.h>

#define SKIPLIST_MAX_LEVELS 6
#define SKIPLIST_MAX_SIZE 0xFFFFFFFF
#define SKIPLIST_DEFAULT_PROB 0.5f

struct skiplist_node {
	char *key; //!< Node key;
	u32 hash; //!< Node hash;
	struct skiplist_node **forward; //!< List forward.
};

struct skiplist {
	struct object obj; //!< Base object.
	xfiredb_mutex_t lock;

	int level; //!< Current number of levels.
	atomic_t size; //!< Atomic size.
	struct skiplist_node *header; //!< Node header.
	double prob; //!< Skiplist probability
};

struct skiplist_iterator {
	struct skiplist *list;
	struct skiplist_node *current,
			     *prev;
};

#define skiplist_iterator_to_node(__it) (__it)->current
#define skiplist_for_each(__l, __c) \
	for(__c = (__l)->header->forward[0]; __c && __c != (__l)->header; \
			__c  = (__c)->forward[0])

#define skiplist_for_each_safe(__l, __c, __tmp) \
	for(__c = (__l)->header->forward[0], __tmp = (__c)->forward[0]; \
			__c && __c != (__l)->header; \
			__c = __tmp, __tmp = (__c)->forward[0])

CDECL
extern void skiplist_init(struct skiplist *l);
extern void raw_skiplist_init(struct skiplist *l, double p);
extern struct skiplist *skiplist_alloc(void);
extern struct skiplist *raw_skiplist_alloc(double prob);

extern int skiplist_insert(struct skiplist *list, const char *key, struct skiplist_node *node);
extern struct skiplist_node *skiplist_search(struct skiplist *l, const char *key);
extern int skiplist_delete(struct skiplist *list, const char *key);
extern void skiplist_node_destroy(struct skiplist_node *node);
extern void skiplist_dump(FILE *stream, struct skiplist *l);


extern struct skiplist_node *skiplist_iterator_next(struct skiplist_iterator *it);
extern struct skiplist_node *skiplist_iterator_delete(struct skiplist_iterator *it);
extern struct skiplist_iterator *skiplist_iterator_new(struct skiplist *l);
extern void skiplist_iterator_free(struct skiplist_iterator *it);

extern void skiplist_destroy(struct skiplist *l);
extern void skiplist_free(struct skiplist *l);

static inline void skiplist_lock(struct skiplist *list)
{
	xfiredb_mutex_lock(&list->lock);
}

static inline void skiplist_unlock(struct skiplist *list)
{
	xfiredb_mutex_unlock(&list->lock);
}

static inline s32 skiplist_size(struct skiplist *list)
{
	return atomic_get(&list->size);
}
CDECL_END

#endif

/** @} */

