/*
 *  LIST storage
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
 * @addtogroup list
 * @{
 */

#include <stdlib.h>
#include <stdio.h>

#include <xfiredb/xfiredb.h>
#include <xfiredb/types.h>
#include <xfiredb/list.h>
#include <xfiredb/os.h>

static void __list_add(struct list *new, struct list *prev, struct list *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

/**
 * @brief Push an element on the right side of \p head.
 * @param head List head.
 * @param node Node to add to \p head.
 */
void list_rpush(struct list_head *head, struct list *node)
{
	struct list *it;

	it = &head->head;
	it = it->prev;

	__list_add(node, it, it->next);
	atomic_inc(head->num);
}

/**
 * @brief Push an element on the left side of \p head.
 * @param head List head.
 * @param node Node to add to \p head.
 */
void list_lpush(struct list_head *head, struct list *node)
{
	struct list *it;

	it = &head->head;

	__list_add(node, it, it->next);
	atomic_inc(head->num);
}

static inline void __list_del(struct list *prev, struct list *next)
{
	next->prev = prev;
	prev->next = next;
}

/**
 * @brief Delete an entry from a given list.
 * @param lh List to delete from.
 * @param entry Entry which has to be deleted.
 */
void list_del(struct list_head *lh, struct list *entry)
{
	__list_del(entry->prev, entry->next);

	entry->next = entry;
	entry->prev = entry;
	atomic_dec(lh->num);
}

#if 0
void list_rpush(struct list_head *head, struct list *node)
{
	struct list *it;

	xfiredb_spin_lock(&head->lock);
	if(!head->head) {
		head->head = node;
		node->next = NULL;
	} else {
		for(it = head->head; it; it = it->next) {
			if(!it->next)
				break;
		}

		it->next = node;
		node->prev = it;
		node->next = NULL;
	}
	xfiredb_spin_unlock(&head->lock);
}

void list_lpush(struct list_head *head, struct list *node)
{
	struct list *next = head->head;

	xfiredb_spin_lock(&head->lock);
	if(next) {
		next->prev = node;
		node->next = next;
	}

	node->prev= NULL;
	head->head = node;

	xfiredb_spin_unlock(&head->lock);
}
#endif

/** @} */

