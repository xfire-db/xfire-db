/*
 *  XFireDB ruby list
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
#include <ruby.h>

#include <xfire/xfire.h>
#include <xfire/mem.h>
#include <xfire/database.h>
#include <xfire/container.h>
#include <xfire/list.h>
#include <xfire/string.h>
#include <xfire/hashmap.h>

#include "se.h"

VALUE c_hashmap;

VALUE rb_hashmap_clear(VALUE self);

static void rb_hashmap_release(void *p)
{
	struct db_entry_container *container = p;

	rb_hashmap_clear(container->obj);
	container_destroy(&container->c);
	xfire_free(container);
}

void rb_hashmap_free(VALUE obj)
{
	rb_gc_mark(obj);
}

VALUE rb_hashmap_alloc(VALUE klass)
{
	struct db_entry_container *rb_container = xfire_zalloc(sizeof(*rb_container));

	container_init(&rb_container->c, CONTAINER_HASHMAP);
	rb_container->obj = Data_Wrap_Struct(klass, NULL, rb_hashmap_release, rb_container);
	rb_container->type = klass;
	return rb_container->obj;
}

static struct hashmap *obj_to_map(VALUE obj)
{
	struct db_entry_container *c;

	Data_Get_Struct(obj, struct db_entry_container, c);
	return container_get_data(&c->c);
}

static struct hashmap_node *__rb_hashmap_ref(VALUE self, char *key)
{
	struct hashmap *map;

	map = obj_to_map(self);
	return hashmap_find(map, key);
}

static struct hashmap_node *__rb_hashmap_delete(VALUE self, char *key)
{
	struct hashmap *map;

	map = obj_to_map(self);
	return hashmap_remove(map, key);
}

VALUE rb_hashmap_size(VALUE self)
{
	struct hashmap *map;

	map = obj_to_map(self);
	return INT2NUM(hashmap_size(map));
}

VALUE rb_hashmap_clear(VALUE self)
{
	struct hashmap_node *node;
	struct hashmap *map;
	struct string *s;

	map = obj_to_map(self);
	hashmap_clear_foreach(map, node) {
		s = container_of(node, struct string, node);
		hashmap_node_destroy(node);
		string_destroy(s);
		xfire_free(s);
	}

	return self;
}

VALUE rb_hashmap_delete(VALUE self, VALUE key)
{
	struct string *s;
	char *data;
	char *keyval = StringValueCStr(key);
	struct hashmap_node *node;
	VALUE rv = Qnil;

	node = __rb_hashmap_delete(self, keyval);
	if(!node && rb_block_given_p())
		return rb_yield(key);
	if(!node)
		return Qnil;

	s = container_of(node, struct string, node);
	string_get(s, &data);
	rv = rb_str_new2(data);
	xfire_free(data);
	hashmap_node_destroy(node);
	string_destroy(s);
	xfire_free(s);

	return rv;
}

VALUE rb_hashmap_ref(VALUE self, VALUE key)
{
	struct string *s;
	char *tmp;
	char *keyval = StringValueCStr(key);
	struct hashmap_node *node;
	VALUE rv = Qnil;

	node = __rb_hashmap_ref(self, keyval);
	if(!node)
		return Qnil;

	s = container_of(node, struct string, node);
	string_get(s, &tmp);
	rv = rb_str_new2(tmp);
	xfire_free(tmp);

	return rv;
}

VALUE rb_hashmap_store(VALUE self, VALUE key, VALUE data)
{
	struct string *s;
	char *tmp_key = StringValueCStr(key);
	char *tmp_data = StringValueCStr(data);
	struct hashmap_node *node;
	struct hashmap *map;

	map = obj_to_map(self);
	node = hashmap_find(map, tmp_key);

	if(!node) {
		s = string_alloc(tmp_data);
		hashmap_add(map, tmp_key, &s->node);
	} else {
		s = container_of(node, struct string, node);
		string_set(s, tmp_data);
	}

	return data;
}

VALUE rb_hashmap_new(void)
{
	return rb_hashmap_alloc(c_hashmap);
}

VALUE hash_enum_size(VALUE hash, VALUE args, VALUE obj)
{
	return rb_hashmap_size(hash);
}

VALUE rb_hashmap_each(VALUE hash)
{
	RETURN_SIZED_ENUMERATOR(hash, 0, 0, hash_enum_size);
	hashmap_iterator_t it;
	struct hashmap *map;
	struct hashmap_node *node;
	VALUE key, value;
	char *tmp;
	struct string *s;

	map = obj_to_map(hash);
	it = hashmap_new_iterator(map);

	for(node = hashmap_iterator_next(it); node;
			node = hashmap_iterator_next(it)) {
		s = container_of(node, struct string, node);
		string_get(s, &tmp);

		value = rb_str_new2(tmp);
		key = rb_str_new2(node->key);
		xfire_free(tmp);

		rb_yield(rb_assoc_new(key, value));
	}

	return hash;
}

void init_hashmap(void)
{
	c_hashmap = rb_define_class_under(c_xfiredb_mod,
			"Hashmap", rb_cObject);
	rb_include_module(c_hashmap, rb_mEnumerable);
	rb_define_singleton_method(c_hashmap, "new", rb_hashmap_alloc, 0);

	rb_define_method(c_hashmap, "store", rb_hashmap_store, 2);
	rb_define_method(c_hashmap, "[]=", rb_hashmap_store, 2);
	rb_define_method(c_hashmap, "[]", rb_hashmap_ref, 1);
	rb_define_method(c_hashmap, "delete", rb_hashmap_delete, 1);
	rb_define_method(c_hashmap, "clear", rb_hashmap_clear, 0);
	rb_define_method(c_hashmap, "size", rb_hashmap_size, 0);
	rb_define_method(c_hashmap, "each", rb_hashmap_each, 0);
}

