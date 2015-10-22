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

#include <xfire/xfire.h>
#include <xfire/error.h>

static VALUE c_storage_engine;
static VALUE c_se_mod;

static VALUE rb_se_new(VALUE class)
{
	fprintf(stdout, "Hello, storage engine\n");
}

void Init_storage_engine(void)
{
	c_se_mod = rb_define_module("XFireDB");
	c_storage_engine = rb_define_class_under(c_se_mod,
			"StorageEngine", rb_cObject);
	rb_define_singleton_method(c_storage_engine, "new", &rb_se_new, 0);
}

