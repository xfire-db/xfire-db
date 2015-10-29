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

/**
 * @addtogroup container
 * @{
 */

#include <xfiredb/xfiredb.h>
#include <xfiredb/error.h>
#include <xfiredb/types.h>
#include <xfiredb/container.h>
#include <xfiredb/string.h>
#include <xfiredb/list.h>
#include <xfiredb/hashmap.h>
#include <xfiredb/mem.h>

/**
 * @brief Initialise a new container.
 * @param c Container to initialise.
 * @param type Container type.
 */
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

/**
 * @brief Allocate a new container.
 * @param type Container type to allocate.
 * @return The allocated container.
 */
struct container *container_alloc(container_type_t type)
{
	struct container *c;

	c = xfire_zalloc(sizeof(*c));
	container_init(c, type);

	return c;
}

/**
 * @brief Get the data encapsulated by the container.
 * @return The encapsulated data.
 */
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

/**
 * @brief Destroy a given container.
 * @param c Cotainer to destroy.
 */
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

/** @} */

