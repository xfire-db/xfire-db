/*
 *  XFireDB set
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

#include <xfiredb/xfiredb.h>
#include <xfiredb/mem.h>
#include <xfiredb/database.h>
#include <xfiredb/container.h>
#include <xfiredb/bio.h>
#include <xfiredb/set.h>

#include "se.h"

VALUE c_set;

void rb_set_remove(struct db_entry_container *e)
{
	struct container *c = &e->c;
	struct set *set;

	set = container_get_data(c);
	set_clear(set);
}

void rb_set_free(struct db_entry_container *e)
{
	struct container *c = &e->c;

	rb_set_remove(e);
	if(!e->intree && e->obj_released) {
		container_destroy(c);
		xfire_free(e->key);
		xfire_free(e);
	}
}

static void rb_set_release(void *p)
{
	struct db_entry_container *e = p;

	e->obj_released = true;
	if(!e->intree) {
		rb_set_remove(e);
		container_destroy(&e->c);
		xfire_free(e->key);
		xfire_free(e);
	}
}

static VALUE rb_set_alloc(VALUE klass)
{
	struct db_entry_container *container = xfire_zalloc(sizeof(*container));
	VALUE obj;

	container_init(&container->c, CONTAINER_SET);
	obj = Data_Wrap_Struct(klass, NULL, rb_set_release, container);
	container->intree = false;
	container->type = klass;
	container->obj_released = false;
	container->release = rb_set_release;

	return obj;
}

static inline struct set *obj_to_set(VALUE obj)
{
	struct db_entry_container *c;

	Data_Get_Struct(obj, struct db_entry_container, c);
	return container_get_data(&c->c);
}

static VALUE rb_set_add(VALUE self, VALUE _key)
{
	struct set *set;
	struct db_entry_container *e;
	char *key = StringValueCStr(_key);
	struct set_key *k = xfire_zalloc(sizeof(*k));

	Data_Get_Struct(self, struct db_entry_container, e);
	set = obj_to_set(self);
	if(set_add(set, key, k) == -XFIRE_OK) {
		if(e->key)
			xfiredb_notice_disk(key, k->key, NULL, SET_ADD);

		return _key;
	}

	return Qnil;
}

static VALUE rb_set_remove_key(VALUE self, VALUE _key)
{
	struct set *set;
	struct set_key *k;
	char *key = StringValueCStr(_key);
	struct db_entry_container *e;

	Data_Get_Struct(self, struct db_entry_container, e);
	set = obj_to_set(self);
	k = set_remove(set, key);

	if(!k)
		return Qnil;

	if(e->key)
		xfiredb_notice_disk(e->key, k->key, NULL, SET_DEL);
	set_key_destroy(k);
	xfire_free(k);
	return _key;
}

static VALUE rb_set_clear(VALUE self)
{
	struct set *set = obj_to_set(self);
	struct set_key *k;
	struct set_iterator *it;
	struct db_entry_container *e;

	Data_Get_Struct(self, struct db_entry_container, e);
	if(e->key) {
		it = set_iterator_new(set);
		for_each_set(set, k, it)
			xfiredb_notice_disk(e->key, k->key, NULL, SET_DEL);

		set_iterator_free(it);
	}

	set_clear(set);
	return self;
}

static VALUE rb_set_include(VALUE self, VALUE _key)
{
	struct set *set;
	char *key = StringValueCStr(_key);

	set = obj_to_set(self);
	return set_contains(set, key) ? Qtrue : Qfalse;
}

static VALUE rb_set_size(VALUE self)
{
	int rv;
	struct set *set = obj_to_set(self);

	rv = set_size(set);
	return INT2NUM(rv);
}

static VALUE set_enum_size(VALUE self)
{
	return rb_set_size(self);
}

static VALUE rb_set_each(VALUE set)
{
	RETURN_SIZED_ENUMERATOR(set, 0, 0, set_enum_size);
	struct set *s;
	struct set_key *k;
	struct set_iterator *it;

	s = obj_to_set(set);
	it = set_iterator_new(s);
	for_each_set(s, k, it)
		rb_yield(rb_str_new2(k->key));

	set_iterator_free(it);
	return set;
}

void init_set(void)
{
	c_set = rb_define_class_under(c_xfiredb_mod, "Set", rb_cObject);
	rb_include_module(c_set, rb_mEnumerable);

	rb_define_singleton_method(c_set, "new", rb_set_alloc, 0);
	rb_define_method(c_set, "add", rb_set_add, 1);
	rb_define_method(c_set, "remove", rb_set_remove_key, 1);
	rb_define_method(c_set, "clear", rb_set_clear, 0);
	rb_define_method(c_set, "size", rb_set_size, 0);
	rb_define_method(c_set, "each", rb_set_each, 0);
	rb_define_method(c_set, "include?", rb_set_include, 1);
}

