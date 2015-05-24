/*
 *  LIST storage header
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

#ifndef __LIST_H__
#define __LIST_H__

#include <xfire/xfire.h>
#include <xfire/os.h>

typedef struct list {
	struct list *next;
	struct list *prev;
} LIST;

typedef struct list_head {
	struct list head;

	xfire_spinlock_t lock;
	atomic_t num;
} LIST_HEAD;

#define LH_MAGIC 0x87A5F947

#define list_for_each(__lh, __c) \
	for(__c = (__lh)->head.next; __c != &((__lh)->head); __c = __c->next)

#define list_for_each_safe(__lh, __c, __s) \
	for(__c = (__lh)->head.next, __s = __c->next; __c != &((__lh)->head); \
			__c = __s, __s = __c->next)

CDECL
extern void list_lpush(struct list_head *head, struct list *node);
extern void list_rpush(struct list_head *head, struct list *node);
extern void list_pop(struct list_head *head, u32 idx);
extern void list_del(struct list_head *lh, struct list *entry);

static inline void list_node_init(struct list *node)
{

	node->next = node;
	node->prev = node;
}

static inline void list_head_init(struct list_head *head)
{
	xfire_spinlock_init(&head->lock);
	list_node_init(&head->head);

	atomic_init(&head->num);
}

static inline void list_head_destroy(struct list_head *head)
{
	xfire_spinlock_destroy(&head->lock);
}

static inline int list_length(struct list_head *head)
{
	return atomic_get(&head->num);
}

static inline void list_lock(struct list_head *lh)
{
	xfire_spin_lock(&lh->lock);
}

static inline void list_unlock(struct list_head *lh)
{
	xfire_spin_unlock(&lh->lock);
}
CDECL_END

#endif

