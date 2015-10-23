/*
 *  XFireDB interface
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
#include <stdio.h>
#include <ruby.h>

#include "se.h"

#include <xfire/xfire.h>
#include <xfire/error.h>
#include <xfire/mem.h>

static VALUE c_storage_engine;
static VALUE c_se_mod;

static VALUE rb_se_new(VALUE class)
{
	xfiredb_init();
	return class;
}

static VALUE rb_se_delete(VALUE self)
{
	xfiredb_exit();
	return self;
}

static VALUE rb_se_key_del(VALUE self, VALUE _key)
{
	char *key = StringValueCStr(_key);

	return INT2NUM(xfiredb_key_delete(key));
}

static VALUE rb_se_disk_clear(VALUE self)
{
	xfiredb_disk_clear();
	return self;
}

void Init_storage_engine(void)
{
	c_se_mod = rb_define_module("XFireDB");
	c_storage_engine = rb_define_class_under(c_se_mod,
			"StorageEngine", rb_cObject);
	rb_define_method(c_storage_engine, "initialize", &rb_se_new, 0);
	rb_define_method(c_storage_engine, "delete", &rb_se_delete, 0);
	/* string funcs */
	rb_define_method(c_storage_engine, "string_set", &rb_se_str_set, 2);
	rb_define_method(c_storage_engine, "string_get", &rb_se_str_get, 1);
	/* list funcs */
	rb_define_method(c_storage_engine, "list_get_seq", &rb_se_list_get, 3);
	rb_define_method(c_storage_engine, "list_get", &rb_se_list_get2, 2);
	rb_define_method(c_storage_engine, "list_pop_seq", &rb_se_list_del, 3);
	rb_define_method(c_storage_engine, "list_pop", &rb_se_list_del2, 2);
	rb_define_method(c_storage_engine, "list_lpush", &rb_se_list_lpush, 2);
	rb_define_method(c_storage_engine, "list_rpush", &rb_se_list_rpush, 2);
	rb_define_method(c_storage_engine, "list_set", &rb_se_list_set, 3);
	/* hm funcs */
	rb_define_method(c_storage_engine, "hm_get", &rb_se_hm_get, 2);
	rb_define_method(c_storage_engine, "hm_set", &rb_se_hm_set, 3);
	rb_define_method(c_storage_engine, "hm_remove", &rb_se_hm_del, 2);
	/* generic funcs */
	rb_define_method(c_storage_engine, "key_delete", &rb_se_key_del, 1);
	rb_define_method(c_storage_engine, "disk_clear", &rb_se_disk_clear, 0);
}

