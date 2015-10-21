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

#include <xfire/xfire.h>
#include <xfire/error.h>
#include <xfire/types.h>
#include <xfire/container.h>
#include <xfire/string.h>
#include <xfire/list.h>
#include <xfire/hashmap.h>
#include <xfire/mem.h>

void container_init(struct container *c, container_type_t type)
{
	switch(type) {
	case CONTAINER_STRING:
		string_init(&c->data.string);
		break;
	case CONTAINER_LIST:
		list_head_init(&c->data.list);
		break;
	case CONTAINER_HASHMAP:
		hashmap_init(&c->data.map);
		break;
	default:
		break;
	}

	c->type = type;
}

struct container *container_alloc(container_type_t type)
{
	struct container *c;

	c = xfire_zalloc(sizeof(*c));
	container_init(c, type);

	return c;
}

void *container_get_data(struct container *c)
{
	void *data;

	switch(c->type) {
	case CONTAINER_STRING:
		data = &c->data.string;
		break;
	case CONTAINER_LIST:
		data = &c->data.list;
		break;
	case CONTAINER_HASHMAP:
		data = &c->data.map;
		break;
	default:
		data = NULL;
		break;
	}

	return data;
}

void container_destroy(struct container *c)
{
	switch(c->type) {
	case CONTAINER_STRING:
		string_destroy(&c->data.string);
		break;
	case CONTAINER_LIST:
		list_head_destroy(&c->data.list);
		break;
	case CONTAINER_HASHMAP:
		hashmap_destroy(&c->data.map);
		break;
	default:
		break;
	}
}

