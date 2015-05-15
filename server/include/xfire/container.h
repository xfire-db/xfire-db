/*
 *  CONTAINER header
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

#ifndef __CONTAINER__H__
#define __CONTAINER__H__

#include <xfire/xfire.h>
#include <xfire/types.h>
#include <xfire/rbtree.h>
#include <xfire/list.h>
#include <xfire/string.h>

typedef enum container_type {
	LIST_CONTAINER,
	STRING_CONTAINER,
} container_type_t;

typedef struct container {
	u32 magic;
	struct rb_node node;
	char *key;

	union {
		struct list_head lh;
		struct string string;
	} data;
} CONTAINER;

CDECL
extern void *node_get_data(struct rb_node *node, u32 type);
extern void *container_get_data(struct container *c, u32 type);
extern void container_init(struct container *c, const char *key, u32 magic);
extern void container_destroy(struct container *c, u32 type);
extern void container_set_string(struct container *c, void *data);
CDECL_END
#endif

