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

VALUE rb_se_str_get(VALUE self, VALUE _key)
{
	char *key, *data;

	key = StringValueCStr(_key);
	if(xfiredb_string_get(key, &data) != -XFIRE_OK)
		return Qnil;

	return rb_str_new2(data);
}

VALUE rb_se_str_set(VALUE self, VALUE _key, VALUE data)
{
	char *key, *str;

	key = StringValueCStr(_key);
	str = StringValueCStr(data);
	return xfiredb_string_set(key, str) == -XFIRE_OK ?
		Qtrue : Qfalse;
}

