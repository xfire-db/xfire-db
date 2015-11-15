/*
 *  XFireDB ruby string
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
#include <xfiredb/error.h>
#include <xfiredb/quotearg.h>

#include "se.h"

static VALUE rb_string_escape(VALUE str)
{
	char *x, *new;
	VALUE rv;

	x = StringValueCStr(str);
	new = xfiredb_escape_string(x);
	rv = rb_str_new2(new);
	xfiredb_escape_free(new);

	return rv;
}

static VALUE rb_string_unescape(VALUE str)
{
	char *x, *new;
	VALUE rv;

	x = StringValueCStr(str);
	new = xfiredb_unescape_string(x);
	rv = rb_str_new2(new);
	xfiredb_escape_free(new);

	return rv;
}

static VALUE rb_string_singleton_escape(VALUE klass, VALUE str)
{
	return rb_string_escape(str);
}

static VALUE rb_string_singleton_unescape(VALUE klass, VALUE str)
{
	return rb_string_unescape(str);
}

void init_string(void)
{
	rb_define_method(rb_cString, "escape", rb_string_escape, 0);
	rb_define_method(rb_cString, "unescape", rb_string_unescape, 0);
	rb_define_singleton_method(rb_cString, "escape", rb_string_singleton_escape, 1);
	rb_define_singleton_method(rb_cString, "unescape", rb_string_singleton_unescape, 1);
}

