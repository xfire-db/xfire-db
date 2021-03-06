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

#include <xfiredb/xfiredb.h>
#include <xfiredb/error.h>
#include <xfiredb/mem.h>
#include <xfiredb/database.h>
#include <xfiredb/disk.h>

extern void init_list(void);
extern void init_database(void);
extern void init_hashmap(void);

VALUE rb_se_init(VALUE self,
		VALUE log_file,
		VALUE err_log,
		VALUE db_file,
		VALUE pers_lvl,
		VALUE silent)
{
	struct config conf;

	xfiredb_sprintf(&conf.log_file, "%s", StringValueCStr(log_file));
	xfiredb_sprintf(&conf.err_log_file, "%s", StringValueCStr(err_log));
	xfiredb_sprintf(&conf.db_file, "%s", StringValueCStr(db_file));
	conf.persist_level = NUM2INT(pers_lvl);

	if(silent == Qtrue)
		xfiredb_se_init_silent(&conf);
	else
		xfiredb_se_init(&conf);

	return self;
}

VALUE rb_se_set_loadstate(VALUE self, VALUE state)
{
	if(state == Qtrue)
		xfiredb_set_loadstate(true);
	else
		xfiredb_set_loadstate(false);

	return state;
}

VALUE rb_se_get_loadstate(VALUE self)
{
	if(xfiredb_loadstate())
		return Qtrue;
	else
		return Qfalse;
}

VALUE rb_se_load_key(VALUE self, VALUE _key)
{
	VALUE ary;
	auto void load_hook(int argc, char **rows, char **cols);
	char *k = StringValueCStr(_key);

	void load_hook(int argc, char **rows, char **cols)
	{
		VALUE key, hash, type, data, tmp;
		int i;

		for(i = 0; i < argc; i += 4) {
			key = rb_str_new2(rows[i + TABLE_KEY_IDX]);
			hash = rb_str_new2(rows[i + TABLE_SCND_KEY_IDX]);
			type = rb_str_new2(rows[i + TABLE_TYPE_IDX]);
			data = rb_str_new2(rows[i + TABLE_DATA_IDX]);

			tmp = rb_ary_new_from_args(4, key, hash, type, data);
			rb_ary_push(ary, tmp);
		}
	}

	ary = rb_ary_new();
	xfiredb_load_key(k, &load_hook);
	return ary;
}

VALUE rb_se_load(VALUE self)
{
	auto void load_hook(int argc, char **rows, char **cols);
	VALUE ary;

	void load_hook(int argc, char **rows, char **cols)
	{
		VALUE key, hash, type, data, tmp;
		int i;

		for(i = 0; i < argc; i += 4) {
			key = rb_str_new2(rows[i + TABLE_KEY_IDX]);
			hash = rb_str_new2(rows[i + TABLE_SCND_KEY_IDX]);
			type = rb_str_new2(rows[i + TABLE_TYPE_IDX]);
			data = rb_str_new2(rows[i + TABLE_DATA_IDX]);

			tmp = rb_ary_new_from_args(4, key, hash, type, data);
			rb_ary_push(ary, tmp);
		}
	}

	ary = rb_ary_new();
	xfiredb_raw_load(&load_hook);
	return ary;
}

VALUE rb_se_exit(VALUE self, VALUE db)
{
	xfiredb_se_exit();
	return self;
}

VALUE rb_se_save(VALUE self)
{
	xfiredb_se_save();
	return Qnil;
}

VALUE c_xfiredb_mod;
VALUE rb_cStorageEngine;
void Init_storage_engine(void)
{
	c_xfiredb_mod = rb_define_module("XFireDB");
	rb_cStorageEngine = rb_define_class_under(c_xfiredb_mod,
			"Engine", rb_cObject);

	rb_define_method(rb_cStorageEngine, "init", rb_se_init, 5);
	rb_define_method(rb_cStorageEngine, "stop", rb_se_exit, 1);
	rb_define_method(rb_cStorageEngine, "save", rb_se_save, 0);
	rb_define_method(rb_cStorageEngine, "load", rb_se_load, 0);
	rb_define_method(rb_cStorageEngine, "load_key", rb_se_load_key, 1);
	rb_define_method(rb_cStorageEngine, "set_loadstate", rb_se_set_loadstate, 1);
	rb_define_method(rb_cStorageEngine, "get_loadstate", rb_se_get_loadstate, 0);

	init_database();
	init_list();
	init_hashmap();
	init_set();
	init_string();
	init_log();
	init_digest();
}

