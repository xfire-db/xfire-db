/*
 *  XFireDB list interface
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

#include <xfire/xfire.h>
#include <xfire/error.h>
#include <xfire/mem.h>

VALUE rb_se_hm_set(VALUE self, VALUE _key, VALUE _skey, VALUE _data)
{
	char *key, *skey, *data;

	key = StringValueCStr(_key);
	skey = StringValueCStr(_skey);
	data = StringValueCStr(_data);

	return xfiredb_hashmap_set(key, skey, data) == -XFIRE_OK ?
		Qtrue : Qfalse;
}

VALUE rb_se_hm_del(VALUE self, VALUE _key, VALUE _skeys)
{
	char *key, **skeys;
	int num = RARRAY_LEN(_skeys), i;
	VALUE key_entry, rv;

	key = StringValueCStr(_key);
	skeys = xfire_zalloc(sizeof(*skeys) * num);

	for(i = 0; i < num; i++) {
		key_entry = rb_ary_entry(_skeys, i);
		skeys[i] = StringValueCStr(key_entry);
	}

	rv = INT2NUM(xfiredb_hashmap_remove(key, skeys, num));
	xfire_free(skeys);

	return rv;
}

VALUE rb_se_hm_clear(VALUE self, VALUE _key)
{
	auto void pop_hook(char *key, char *data);
	VALUE hash = rb_hash_new();
	char *key = StringValueCStr(_key);

	if(xfiredb_hashmap_clear(key, pop_hook) != -XFIRE_OK)
		return Qnil;

	return hash;

	void pop_hook(char *__key, char *data)
	{
		VALUE rbkey, rbdata;

		rbkey = rb_str_new2(__key);
		rbdata = rb_str_new2(data);
		rb_hash_aset(hash, rbkey, rbdata);
	}
}

VALUE rb_se_hm_get(VALUE self, VALUE _key, VALUE _skeys)
{
	char *key, **skeys, **data;
	int num = RARRAY_LEN(_skeys), i;
	VALUE rv, key_entry;

	key = StringValueCStr(_key);
	skeys = xfire_zalloc(sizeof(*skeys) * num);
	data = xfire_zalloc(sizeof(*data) * num);

	for(i = 0; i < num; i++) {
		key_entry = rb_ary_entry(_skeys, i);
		skeys[i] = StringValueCStr(key_entry);
	}

	if(xfiredb_hashmap_get(key, skeys, data, num) != -XFIRE_OK) {
		xfire_free(skeys);
		xfire_free(data);
		return Qnil;
	}

	rv = rb_ary_new();
	for(i = 0; i < num; i++) {
		if(!data[i]) {
			rb_ary_push(rv, rb_str_new2("(nil)"));
		} else {
			rb_ary_push(rv, rb_str_new2(data[i]));
			xfire_free(data[i]);
		}
	}

	xfire_free(skeys);
	xfire_free(data);
	return rv;
}

