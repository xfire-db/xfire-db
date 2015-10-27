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

VALUE rb_se_list_del(VALUE self, VALUE _key,
			    VALUE _start, VALUE _end)
{
	int start, end, length,
	    delta, *idxs, i;
	char *key;
	VALUE rv;

	start = NUM2INT(_start);
	end = NUM2INT(_end);
	key = StringValueCStr(_key);
	length = xfiredb_list_length(key);

	if(!length || start < 0)
		return INT2NUM(0);

	if(end < 0)
		end += length;

	delta = end - start + 1;
	idxs = xfire_zalloc(sizeof(*idxs)*delta);
	for(i = 0; i < delta; i++)
		idxs[i] = start+i;

	rv = INT2NUM(xfiredb_list_pop(key, idxs, delta));
	xfire_free(idxs);

	return rv;
}

VALUE rb_se_list_del2(VALUE self, VALUE _key,
			      VALUE indexes)
{
	int num = RARRAY_LEN(indexes), i;
	char *key = StringValueCStr(_key);
	int *idxs = xfire_zalloc(sizeof(*idxs) * num);
	VALUE rv;

	for(i = 0; i < num; i++)
		idxs[i] = NUM2INT(rb_ary_entry(indexes, i));

	rv = INT2NUM(xfiredb_list_pop(key, idxs, num));
	xfire_free(idxs);
	return rv;
}

VALUE rb_se_list_set(VALUE self, VALUE _key,
			    VALUE index, VALUE _data)
{
	char *key, *data;
	int idx;

	key = StringValueCStr(_key);
	data = StringValueCStr(_data);
	idx = NUM2INT(index);

	return xfiredb_list_set(key, idx, data) == -XFIRE_OK ?
		Qtrue : Qfalse;
}

static VALUE rb_se_list_push(VALUE self, VALUE _key,
			     VALUE _data, bool left)
{
	char *key, *data;

	key = StringValueCStr(_key);
	data = StringValueCStr(_data);
	return xfiredb_list_push(key, data, left) == XFIRE_OK ?
		Qtrue : Qfalse;
	
}

VALUE rb_se_list_rpush(VALUE self, VALUE _key,
			      VALUE _data)
{
	return rb_se_list_push(self, _key, _data, false);
}

VALUE rb_se_list_lpush(VALUE self, VALUE _key,
			      VALUE _data)
{
	return rb_se_list_push(self, _key, _data, true);
}

VALUE rb_se_list_clear(VALUE self, VALUE _key)
{
	auto void clear_hook(char *__key, char *data);
	VALUE ary = rb_ary_new();

	if(xfiredb_list_clear(StringValueCStr(_key), clear_hook) != -XFIRE_OK) {
		rb_ary_free(ary);
		return Qnil;
	}

	return ary;

	void clear_hook(char *__key, char *data)
	{
		VALUE rbdata;

		rbdata = rb_str_new2(data);
		rb_ary_push(ary, rbdata);
	}
}

VALUE rb_se_list_get2(VALUE self, VALUE _key,
			      VALUE indexes)
{
	int num = RARRAY_LEN(indexes), i;
	char **data = xfire_zalloc(sizeof(*data) * num);
	char *key = StringValueCStr(_key);
	int *idxs = xfire_zalloc(sizeof(*idxs) * num);
	VALUE rv;

	for(i = 0; i < num; i++)
		idxs[i] = NUM2INT(rb_ary_entry(indexes, i));

	if(xfiredb_list_get(key, data, idxs, num) != -XFIRE_OK) {
		xfire_free(data);
		xfire_free(idxs);
		return Qnil;
	} else {
		rv = rb_ary_new();

		for(i = 0; i < num; i++) {
			if(!data[i]) {
				rb_ary_push(rv, rb_str_new2("(nil)"));
			} else {
				rb_ary_push(rv, rb_str_new2(data[i]));
				xfire_free(data[i]);
			}
		}

		xfire_free(data);
		xfire_free(idxs);
	}

	return rv;
}

VALUE rb_se_list_get(VALUE self, VALUE _key,
			    VALUE _start, VALUE _end)
{
	int start, end, length, delta, *idxs, i;
	char *key, **data;
	VALUE rv;

	start = NUM2INT(_start);
	end = NUM2INT(_end);
	key = StringValueCStr(_key);
	length = xfiredb_list_length(key);

	if(!length || start < 0)
		return Qnil;

	if(end < 0)
		end += length;

	delta = end - start + 1;
	data = xfire_zalloc(sizeof(*data)*delta);
	idxs = xfire_zalloc(sizeof(*idxs)*delta);
	for(i = 0; i < delta; i++)
		idxs[i] = start+i;

	if(xfiredb_list_get(key, data, idxs, delta) != -XFIRE_OK) {
		xfire_free(data);
		xfire_free(idxs);
		return Qnil;
	} else {
		/* Lookup was succesful */
		rv = rb_ary_new();
		for(i = 0; i < delta; i++) {
			if(!data[i]) {
				rb_ary_push(rv, rb_str_new2("(nil)"));
			} else {
				rb_ary_push(rv, rb_str_new2(data[i]));
				xfire_free(data[i]);
			}
		}

		xfire_free(data);
		xfire_free(idxs);
	}

	return rv;
}

