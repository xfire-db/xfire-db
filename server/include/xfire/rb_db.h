/*
 *  RB DATABASE
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

#ifndef __RB_DB__H__
#define __RB_DB__H__

#include <xfire/xfire.h>
#include <xfire/types.h>
#include <xfire/rbtree.h>
#include <xfire/engine.h>

typedef struct rb_database {
	struct rb_root root;
	struct database db;
} RB_DB;

typedef struct rbdb_node {
	struct rb_node node;
	void *data;
} RBDB_NODE;

#endif
