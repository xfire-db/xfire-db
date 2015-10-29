/*
 *  XFireDB storage engine header
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

#ifndef __DEFINED_SE_H__
#define __DEFINED_SE_H__

#include <stdlib.h>
#include <ruby.h>

#include <xfiredb/engine/xfiredb.h>
#include <xfiredb/engine/container.h>

struct db_entry_container {
	char *key;

	struct container c;
	bool intree;
	bool obj_released;
	VALUE obj;
	VALUE type;
	void (*release)(void *p);
};

extern VALUE c_xfiredb_mod;
extern VALUE c_hashmap;
extern VALUE c_list;

/* hashmap funcs */
extern VALUE rb_hashmap_each(VALUE hash);
extern VALUE rb_hashmap_store(VALUE self, VALUE key, VALUE data);
extern VALUE rb_hashmap_ref(VALUE self, VALUE key);
extern VALUE rb_hashmap_delete(VALUE self, VALUE key);
extern VALUE rb_hashmap_clear(VALUE self);
extern VALUE rb_hashmap_size(VALUE self);
extern void rb_hashmap_free(struct db_entry_container *c);
extern VALUE rb_hashmap_new(void);
extern void rb_hashmap_remove(struct db_entry_container *c);

/* list funcs */
extern void rb_list_free(struct db_entry_container *container);

extern VALUE rb_se_list_del(VALUE self, VALUE _key, VALUE _start, VALUE _end);
extern VALUE rb_se_list_del2(VALUE self, VALUE _key, VALUE indexes);
extern VALUE rb_se_list_set(VALUE self, VALUE _key, VALUE index, VALUE _data);
extern VALUE rb_se_list_rpush(VALUE self, VALUE _key, VALUE _data);
extern VALUE rb_se_list_lpush(VALUE self, VALUE _key, VALUE _data);
extern VALUE rb_se_list_get2(VALUE self, VALUE _key, VALUE indexes);
extern VALUE rb_se_list_get(VALUE self, VALUE _key, VALUE _start, VALUE _end);
extern VALUE rb_se_list_clear(VALUE self, VALUE _key);
/* string funcs */
extern VALUE rb_se_str_set(VALUE self, VALUE _key, VALUE data);
extern VALUE rb_se_str_get(VALUE self, VALUE _key);
/* hm funcs */
extern VALUE rb_se_hm_get(VALUE self, VALUE _key, VALUE _skeys);
extern VALUE rb_se_hm_clear(VALUE self, VALUE _key);
extern VALUE rb_se_hm_set(VALUE self, VALUE _key, VALUE _skey, VALUE _data);
extern VALUE rb_se_hm_del(VALUE self, VALUE _key, VALUE _skeys);
#endif

