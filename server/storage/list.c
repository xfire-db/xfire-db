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

#include <stdlib.h>
#include <stdio.h>

#include <xfire/xfire.h>
#include <xfire/types.h>
#include <xfire/list.h>
#include <xfire/os.h>

static void __list_add(struct list *new, struct list *prev, struct list *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

void list_rpush(struct list_head *head, struct list *node)
{
	struct list *it;

	xfire_spin_lock(&head->lock);
	it = &head->head;
	it = it->prev;

	__list_add(node, it, it->next);
	xfire_spin_unlock(&head->lock);

	atomic_inc(head->num);
}

void list_lpush(struct list_head *head, struct list *node)
{
	struct list *it;

	xfire_spin_lock(&head->lock);
	it = &head->head;

	__list_add(node, it, it->next);
	xfire_spin_unlock(&head->lock);

	atomic_inc(head->num);
}

static inline void __list_del(struct list *prev, struct list *next)
{
	next->prev = prev;
	prev->next = next;
}

void list_del(struct list_head *lh, struct list *entry)
{
	xfire_spin_lock(&lh->lock);
	__list_del(entry->prev, entry->next);

	entry->next = entry;
	entry->prev = entry;
	xfire_spin_unlock(&lh->lock);
	atomic_dec(lh->num);
}

#if 0
void list_rpush(struct list_head *head, struct list *node)
{
	struct list *it;

	xfire_spin_lock(&head->lock);
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
	xfire_spin_unlock(&head->lock);
}

void list_lpush(struct list_head *head, struct list *node)
{
	struct list *next = head->head;

	xfire_spin_lock(&head->lock);
	if(next) {
		next->prev = node;
		node->next = next;
	}

	node->prev= NULL;
	head->head = node;

	xfire_spin_unlock(&head->lock);
}
#endif

