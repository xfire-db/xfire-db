/*
 *  CONTAINER
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
#include <xfire/types.h>
#include <xfire/rbtree.h>
#include <xfire/list.h>
#include <xfire/string.h>
#include <xfire/container.h>

void container_init(struct container *c, u32 magic)
{
	switch(magic) {
	case S_MAGIC:
		string_init(&c->data.string);
		break;

	case LH_MAGIC:
		list_head_init(&c->data.lh);
		break;

	default:
		break;
	}

	c->magic = magic;
}

void container_set_string(struct container *c, void *data)
{
	string_set(&c->data.string, data);
}

void container_destroy(struct container *c, u32 type)
{
	rb_node_destroy(&c->node);

	switch(type) {
	case S_MAGIC:
		string_destroy(&c->data.string);
		break;

	case LH_MAGIC:
		list_head_destroy(&c->data.lh);
		break;

	default:
		break;
	}
}

void *node_get_data(struct rb_node *node, u32 type)
{
	struct container *c;

	if(!node)
		return NULL;

	c = container_of(node, struct container, node);

	if(c->magic != type)
		return NULL;

	return container_get_data(c, type);
}

void *container_get_data(struct container *c, u32 type)
{
	void *rv;

	switch(type) {
	case S_MAGIC:
		rv = &c->data.string;
		break;

	case LH_MAGIC:
		rv = &c->data.lh;
		break;

	default:
		rv = NULL;
		break;
	}

	return rv;
}

