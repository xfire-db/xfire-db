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

#include <xfire/xfire.h>
#include <xfire/types.h>
#include <xfire/engine.h>
#include <xfire/mem.h>
#include <xfire/rbtree.h>
#include <xfire/rb_db.h>
#include <xfire/container.h>

static bool eng_compare_db_node(struct rb_node *node, const void *key)
{
	struct container *c;
	struct rbdb_node *dnode;

	dnode = container_of(node, struct rbdb_node, node);
	c = dnode->data;

	return !strcmp(c->key, key);
}

static void rbdb_free(struct database *db)
{
	struct rb_database *rbdb = container_of(db, struct rb_database, db);
	xfire_free(rbdb);
}

static bool rb_db_insert(struct database *db, u64 key, void *data)
{
	struct rbdb_node *node;
	struct rb_database *rbdb;

	node = xfire_zalloc(sizeof(*node));
	rb_init_node(&node->node);
	node->data = data;
	rbdb = container_of(db, struct rb_database, db);

	rb_set_key(&node->node, key);
	rb_insert(&rbdb->root, &node->node, true);

	return true;
}

static bool rb_db_remove(struct database *db, u64 key, void *arg)
{
	struct rb_database *rbdb;
	struct rb_node *node;
	struct rbdb_node *dnode;

	rbdb = container_of(db, struct rb_database, db);
	node = rb_remove(&rbdb->root, key, arg);

	if(!node)
		return false;

	dnode = container_of(node, struct rbdb_node, node);
	xfire_free(dnode);

	return true;
}

static void *rb_db_lookup(struct database *db, u64 key, void *arg)
{
	struct rb_database *rbdb;
	struct rb_node *node;
	struct rbdb_node *dnode;

	rbdb = container_of(db, struct rb_database, db);
	node = rb_find_duplicate(&rbdb->root, key, arg);
	
	if(node) {
		dnode = container_of(node, struct rbdb_node, node);
		return dnode->data;
	} else {
		return NULL;
	}
}

struct rb_database *rbdb_alloc(const char *name)
{
	struct rb_database *db;
	struct database *database;

	db = xfire_zalloc(sizeof(*db));
	rb_init_root(&db->root);
	db->root.cmp = &eng_compare_db_node;
	eng_init_db(&db->db, name);
	eng_add_db(&db->db);

	database = &db->db;
	database->insert = rb_db_insert;
	database->remove = rb_db_remove;
	database->lookup = rb_db_lookup;
	database->free = rbdb_free;

	return db;
}

