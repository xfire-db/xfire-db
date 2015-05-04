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
#include <xfire/rbtree.h>
#include <xfire/os.h>

typedef struct list {
	struct list *next;
	struct list *prev;

	xfire_mutex_t lock;
} LIST;

typedef struct list_head {
	struct list *head;

	struct rb_node node;
	atomic_t num;
} LIST_HEAD;

CDECL
extern void list_lpush(struct list_head *head, struct list *node);
extern void list_rpush(struct list_head *head, struct list *node);
extern void list_pop(struct list_head *head, u32 idx);

static inline void list_node_init(struct list *node)
{
	xfire_mutex_init(&node->lock);

	node->next = NULL;
	node->prev = NULL;
}

static inline void list_head_init(struct list_head *head)
{
	rb_init_node(&head->node);

	atomic_init(&head->num);
	head->head = NULL;
}
CDECL_END

#endif
