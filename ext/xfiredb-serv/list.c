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

#include <xfiredb/xfiredb.h>
#include <xfiredb/mem.h>
#include <xfiredb/database.h>
#include <xfiredb/container.h>
#include <xfiredb/list.h>
#include <xfiredb/string.h>
#include <xfiredb/hashmap.h>
#include <xfiredb/bio.h>

#include "se.h"

VALUE c_list;

static void raw_rb_list_clear(struct db_entry_container *c);

static void rb_list_release(void *p)
{
	struct db_entry_container *container = p;

	container->obj_released = true;
	if(!container->intree) {
		raw_rb_list_clear(container);
		container_destroy(&container->c);
		xfiredb_free(container->key);
		xfiredb_free(container);
	}
}

void rb_list_free(struct db_entry_container *container)
{
	raw_rb_list_clear(container);

	if(container->obj_released && !container->intree) {
		container_destroy(&container->c);
		xfiredb_free(container->key);
		xfiredb_free(container);
	}
}

VALUE rb_list_alloc(VALUE klass)
{
	struct db_entry_container *container = xfiredb_zalloc(sizeof(*container));

	container_init(&container->c, CONTAINER_LIST);
	container->obj = Data_Wrap_Struct(klass, NULL, rb_list_release, container);
	container->type = klass;
	container->obj_released = container->intree = false;
	container->release = rb_list_release;
	return container->obj;
}

VALUE rb_list_new(void)
{
	return rb_list_alloc(c_list);
}

VALUE rb_list_push(VALUE self, VALUE data)
{
	struct string *s;
	char *tmp = StringValueCStr(data);
	struct db_entry_container *c;
	struct list_head *lh;

	s = string_alloc(tmp);
	Data_Get_Struct(self, struct db_entry_container, c);
	if(c->key)
		xfiredb_notice_disk(c->key, NULL, tmp, LIST_ADD);
	lh = container_get_data(&c->c);
	list_rpush(lh, &s->entry);


	return self;
}

static void raw_rb_list_clear(struct db_entry_container *container)
{
	struct list_head *lh;
	struct list *carriage, *tmp;
	struct string *s;
	char *data;

	lh = container_get_data(&container->c);

	list_for_each_safe(lh, carriage, tmp) {
		list_del(lh, carriage);
		s = container_of(carriage, struct string, entry);
		string_get(s, &data);
		if(container->key)
			xfiredb_notice_disk(container->key, data, NULL, LIST_DEL);
		xfiredb_free(data);
		string_destroy(s);
		xfiredb_free(s);
	}
}

VALUE rb_list_clear(VALUE self)
{
	struct db_entry_container *c;

	Data_Get_Struct(self, struct db_entry_container, c);
	raw_rb_list_clear(c);

	return self;
}

VALUE rb_list_length(VALUE self)
{
	struct db_entry_container *c;
	struct list_head *lh;

	Data_Get_Struct(self, struct db_entry_container, c);
	lh = container_get_data(&c->c);

	return INT2NUM(list_length(lh));
}

static VALUE list_enum_length(VALUE list, VALUE args, VALUE obj)
{
	return rb_list_length(list);
}

static struct list *list_ref(struct list_head *lh, int idx)
{
	struct list *carriage;
	int counter = 0;

	list_for_each(lh, carriage) {
		if(counter == idx)
			return carriage;
		counter++;
	}

	return NULL;
}

VALUE rb_list_set(VALUE self, VALUE i, VALUE data)
{
	int idx = NUM2INT(i);
	struct db_entry_container *c;
	struct list_head *lh;
	struct list *carriage;
	struct string *s;
	char *tmp;

	Data_Get_Struct(self, struct db_entry_container, c);
	lh = container_get_data(&c->c);
	carriage = list_ref(lh, idx);

	if(!carriage)
		return Qnil;

	s = container_of(carriage, struct string, entry);
	string_get(s, &tmp);
	if(c->key)
		xfiredb_notice_disk(c->key, tmp, StringValueCStr(data), LIST_UPDATE);
	xfiredb_free(tmp);
	string_set(s, StringValueCStr(data));
	return data;
}

VALUE rb_list_pop(VALUE self, VALUE i)
{
	int idx = NUM2INT(i);
	struct db_entry_container *c;
	struct list_head *lh;
	struct list *carriage;
	struct string *s;
	VALUE rv;
	char *data;

	Data_Get_Struct(self, struct db_entry_container, c);
	lh = container_get_data(&c->c);
	carriage = list_ref(lh, idx);

	if(!carriage)
		return Qnil;

	list_del(lh, carriage);
	s = container_of(carriage, struct string, entry);
	string_get(s, &data);
	rv = rb_str_new2(data);

	if(c->key)
		xfiredb_notice_disk(c->key, data, NULL, LIST_DEL);

	xfiredb_free(data);
	string_destroy(s);
	xfiredb_free(s);

	return rv;
}

VALUE rb_list_ref(VALUE self, VALUE i)
{
	int idx = NUM2INT(i);
	struct db_entry_container *c;
	struct list_head *lh;
	struct list *carriage;
	struct string *s;
	VALUE rv;
	char *data;

	Data_Get_Struct(self, struct db_entry_container, c);
	lh = container_get_data(&c->c);
	carriage = list_ref(lh, idx);

	if(!carriage)
		return Qnil;

	s = container_of(carriage, struct string, entry);
	string_get(s, &data);
	rv = rb_str_new2(data);
	xfiredb_free(data);
	return rv;
}

VALUE rb_list_each(VALUE self)
{
	int i;
	volatile VALUE list = self;

	RETURN_SIZED_ENUMERATOR(list, 0, 0, list_enum_length);
	for(i = 0; i < NUM2INT(rb_list_length(list)); i++) {
		rb_yield(rb_list_ref(list, INT2NUM(i)));
	}

	return list;
}

VALUE rb_list_to_s(VALUE self)
{
	return rb_str_new2("XFireDB::List");
}

void init_list(void)
{
	c_list = rb_define_class_under(c_xfiredb_mod,
			"List", rb_cObject);
	rb_include_module(c_list, rb_mEnumerable);
	rb_define_singleton_method(c_list, "new", rb_list_alloc, 0);
	rb_define_method(c_list, "push", rb_list_push, 1);
	rb_define_method(c_list, "pop", rb_list_pop, 1);
	rb_define_method(c_list, "[]", rb_list_ref, 1);
	rb_define_method(c_list, "[]=", rb_list_set, 2);
	rb_define_method(c_list, "set", rb_list_set, 2);
	rb_define_method(c_list, "length", rb_list_length, 0);
	rb_define_method(c_list, "to_s", rb_list_to_s, 0);
	rb_define_method(c_list, "each", rb_list_each, 0);
	rb_define_method(c_list, "clear", rb_list_clear, 0);
}

