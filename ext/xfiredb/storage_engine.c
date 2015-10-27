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

extern void init_list(void);
extern void init_database(void);
extern void init_hashmap(void);

VALUE c_xfiredb_mod;
VALUE rb_cStorageEngine;
void Init_storage_engine(void)
{
	c_xfiredb_mod = rb_define_module("XFireDB");
	rb_cStorageEngine = rb_define_class_under(c_xfiredb_mod,
			"StorageEngine", rb_cObject);

	init_database();
	init_list();
	init_hashmap();
}

