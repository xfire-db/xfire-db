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

void list_rpush(struct list_head *head, struct list *node)
{
	struct list *it;

	xfire_spin_lock(&head->lock);
	for(it = head->head; it; it = it->next) {
		if(!it->next)
			break;
	}

	it->next = node;
	node->prev = it;
	node->next = NULL;
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

	xfire_spin_lock(&head->lock);
}

void list_pop(struct list_head *head, u32 num)
{
	struct list *it, *prev, *next;
	u32 idx;

	xfire_spin_lock(&head->lock);
	for(it = head->head, idx = 0; it; it = it->next, idx++) {
		if(idx == num)
			break;
	}

	prev = it->prev;
	next = it->next;

	if(prev)
		prev->next = next;

	if(next)
		next->prev = prev;

	it->next = it->prev = NULL;

	xfire_spin_unlock(&head->lock);
}

