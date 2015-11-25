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
#include <xfiredb/log.h>

#include "se.h"

VALUE c_log;

VALUE rb_log_msg(VALUE klass, VALUE msg)
{
	const char *log = StringValueCStr(msg);

	raw_xfiredb_log(log);
	return klass;
}

VALUE rb_log_msg2(int argc, VALUE *args, VALUE self)
{
	const char *msg;
	bool error;
	bool console;

	if(unlikely(argc < 1 || argc > 3))
		rb_raise(rb_eArgError, "wrong number of arguments");

	msg = StringValueCStr(args[0]);

	if(argc < 2)
		error = false;
	else
		error = args[1] == Qtrue;

	if(argc < 3)
		console = false;
	else
		console = args[2] == Qtrue;

	if(error)
		raw_xfiredb_log_err(msg);
	else if(console)
		raw_xfiredb_log_console(msg);
	else
		raw_xfiredb_log(msg);

	return self;
}

void init_log(void)
{
	c_log = rb_define_class_under(c_xfiredb_mod,
			"Log", rb_cObject);

	rb_define_singleton_method(c_log, "<<", rb_log_msg, 1);
	rb_define_singleton_method(c_log, "write", rb_log_msg2, -1);
	rb_define_method(c_log, "<<", rb_log_msg, 1);
	rb_define_method(c_log, "write", rb_log_msg2, -1);
}

