/*
 *  Containers
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

#ifndef __CONTAINER_H__
#define __CONTAINER_H__

#include <xfire/xfire.h>
#include <xfire/error.h>
#include <xfire/types.h>
#include <xfire/list.h>
#include <xfire/string.h>
#include <xfire/hashmap.h>

#define CONTAINER_STRING_MAGIC 0xFFAABBCC
#define CONTAINER_LIST_MAGIC   0xEEAABBCC
#define CONTAINER_HM_MAGIC     0xDDAABBCC

typedef enum {
	CONTAINER_STRING,
	CONTAINER_LIST,
	CONTAINER_HASHMAP,
} container_type_t;

struct container {
	container_type_t type;

	union {
		struct list_head list;
		struct string string;
		struct hashmap map;
	} data;
};

CDECL
static inline bool container_check_type(struct container *c, container_type_t type)
{
	return c->type == type ? true : false;
}

extern void container_init(struct container *c, container_type_t type);
extern void *container_get_data(struct container *c);
extern void container_destroy(struct container *c);
extern struct container *container_alloc(container_type_t type);
CDECL_END

#endif

