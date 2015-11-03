/*
 *  XFireDB ruby database
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
#include <xfiredb/list.h>
#include <xfiredb/string.h>
#include <xfiredb/hashmap.h>

#include "se.h"

static void rb_db_release(void *p)
{
}

static VALUE rb_container_to_obj(struct db_entry_container *c)
{
	if(c->obj_released) {
		c->obj = Data_Wrap_Struct(c->type, NULL, c->release, c);
		c->obj_released = false;
	}

	return c->obj;
}

VALUE rb_db_new(VALUE klass)
{
	struct database *db = db_alloc("xfire-database");
	VALUE obj = Data_Wrap_Struct(klass, NULL, rb_db_release, db);

	rb_gc_mark(obj);
	return obj;
}

VALUE rb_db_ref(VALUE self, VALUE key)
{
	struct database *db;
	struct container *c;
	struct db_entry_container *entry;
	struct string *s;
	char *tmp;
	VALUE rv;
	db_data_t dbdata;

	Data_Get_Struct(self, struct database, db);
	if(db_lookup(db, StringValueCStr(key), &dbdata) != -XFIRE_OK)
		return Qnil;

	c = dbdata.ptr;
	entry = container_of(c, struct db_entry_container, c);

	if(entry->type != rb_cString) {
		return rb_container_to_obj(entry);
	} else {
		s = container_get_data(&entry->c);
		string_get(s, &tmp);
		rv = rb_str_new2(tmp);
		xfire_free(tmp);

		return rv;
	}
}

static void raw_rb_db_delete(struct db_entry_container *entry)
{
	struct container *c = &entry->c;

	entry->intree = false;
	if(entry->type == rb_cString) {
		/* string type, free it here */
		xfiredb_notice_disk(entry->key, NULL, NULL, STRING_DEL);
		container_destroy(c);
		xfire_free(entry->key);
		xfire_free(entry);
		return;
	} else if(entry->type == c_list) {
		rb_list_free(entry);
		entry->obj = Qnil;
	} else if(entry->type == c_hashmap) {
		rb_hashmap_free(entry);
		entry->obj = Qnil;
	}
}

VALUE rb_db_size(VALUE self)
{
	struct database *db;

	Data_Get_Struct(self, struct database, db);
	return LONG2NUM(db_get_size(db));
}

VALUE rb_db_store(VALUE self, VALUE key, VALUE data)
{
	struct database *db;
	struct container *c;
	struct db_entry_container *rb_c;
	struct string *s;
	const char *tmp = StringValueCStr(key);
	db_data_t dbdata;

	Data_Get_Struct(self, struct database, db);
	if(db_delete(db, tmp, &dbdata) == -XFIRE_OK) {
		c = dbdata.ptr;
		rb_c = container_of(c, struct db_entry_container, c);
		if(rb_c->obj != data) {
			if(rb_c->type == rb_cString && rb_obj_class(data) == rb_cString) {
				c = dbdata.ptr;
				s = container_get_data(c);
				string_set(s, StringValueCStr(data));
				return data;
			}

			raw_rb_db_delete(rb_c);
		}
	}

	if(rb_obj_class(data) != rb_cString) {
		Data_Get_Struct(data, struct db_entry_container, rb_c);
		rb_c->obj = data;
		rb_c->intree = true;
	} else {
		rb_c = xfire_zalloc(sizeof(*rb_c));
		rb_c->obj = Qnil;
		rb_c->type = rb_cString;
		container_init(&rb_c->c, CONTAINER_STRING);
		s = container_get_data(&rb_c->c);
		string_set(s, StringValueCStr(data));
	}

	xfire_sprintf(&rb_c->key, "%s", tmp);

	if(db_store(db, tmp, &rb_c->c) != -XFIRE_OK) {
		rb_c->intree = false;
		return Qnil;
	}

	xfiredb_store_container((char*)tmp, &rb_c->c);
	return data;
}

VALUE rb_db_delete(VALUE self, VALUE key)
{
	struct database *db;
	struct container *c;
	struct db_entry_container *entry;
	db_data_t dbdata;

	Data_Get_Struct(self, struct database, db);
	if(db_delete(db, StringValueCStr(key), &dbdata) != -XFIRE_OK)
		return Qnil;

	c = dbdata.ptr;
	entry = container_of(c, struct db_entry_container, c);
	raw_rb_db_delete(entry);

	return key;
}

void rb_db_free(VALUE self)
{
	struct database *db;

	Data_Get_Struct(self, struct database, db);
	db_free(db);
}

static VALUE db_enum_size(VALUE db, VALUE args, VALUE obj)
{
	return rb_db_size(db);
}

static VALUE rb_db_each_pair(VALUE db)
{
	struct db_iterator *it;
	struct db_entry *e;
	struct database *dbase;
	struct container *c;
	struct db_entry_container *db_c;
	char *value;
	VALUE k, v;
	struct string *s_val;

	Data_Get_Struct(db, struct database, dbase);
	RETURN_SIZED_ENUMERATOR(db, 0, 0, db_enum_size);
	it = db_get_iterator(dbase);

	for(e = db_iterator_next(it); e; e = db_iterator_next(it)) {
		c = e->value.ptr;
		db_c = container_of(c, struct db_entry_container, c);
		k = rb_str_new2(e->key);

		if(db_c->type != rb_cString) {
			v = rb_container_to_obj(db_c);
		} else {
			s_val = container_get_data(c);
			string_get(s_val, &value);
			v = rb_str_new2(value);
			xfire_free(value);
		}

		rb_yield(rb_assoc_new(k, v));
	}

	return db;
}

VALUE c_database;

void init_database(void)
{
	c_database = rb_define_class_under(c_xfiredb_mod,
			"Database", rb_cObject);
	rb_include_module(c_database, rb_mEnumerable);
	rb_define_singleton_method(c_database, "new", rb_db_new, 0);
	rb_define_method(c_database, "[]=", rb_db_store, 2);
	rb_define_method(c_database, "[]", rb_db_ref, 1);
	rb_define_method(c_database, "delete", rb_db_delete, 1);
	rb_define_method(c_database, "size", rb_db_size, 0);
	rb_define_method(c_database, "each", rb_db_each_pair, 0);
}

