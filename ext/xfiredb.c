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

#include <xfire/xfire.h>
#include <xfire/types.h>
#include <xfire/bg.h>
#include <xfire/bio.h>
#include <xfire/database.h>
#include <xfire/mem.h>
#include <xfire/os.h>
#include <xfire/error.h>
#include <xfire/disk.h>

static struct database *xfiredb;

struct disk *dbg_disk;
#ifndef HAVE_DEBUG
struct disk *xfire_disk;
#endif

static container_type_t xfiredb_get_row_type(char *cell)
{
	if(!strcmp(cell, "string"))
		return CONTAINER_STRING;
	if(!strcmp(cell, "list"))
		return CONTAINER_LIST;
	if(!strcmp(cell, "hashmap"))
		return CONTAINER_HASHMAP;

	return 0;
}

static void xfiredb_load_hook(int argc, char **rows, char **cols)
{
	container_type_t type;
	int i;

	for(i = 0; i < argc; i += 4) {
		type = xfiredb_get_row_type(rows[i + TABLE_TYPE_IDX]);

		switch(type) {
		case CONTAINER_STRING:
			break;

		case CONTAINER_LIST:
			break;

		case CONTAINER_HASHMAP:
			break;
		}
	}
}

void xfiredb_init(void)
{
	xfiredb = db_alloc("xfiredb");
	bg_processes_init();
	bio_init();
	disk_load(disk_db, &xfiredb_load_hook);
}

void xfiredb_exit(void)
{
	bio_sync();
	bio_exit();
	bg_processes_exit();
	db_free(xfiredb);
}

int xfiredb_string_get(char *key, char **data)
{
	struct string *s;
	struct container *c;
	db_data_t dbdata;

	if(db_lookup(xfiredb, key, &dbdata) != -XFIRE_OK)
		return -XFIRE_ERR;

	c = dbdata.ptr;
	s = container_get_data(c);
	string_get(s, data);
	return -XFIRE_OK;
}

int xfiredb_string_set(char *key, char *str)
{
	struct string *s;
	struct container *c;
	db_data_t data;

	if(!db_lookup(xfiredb, key, &data)) {
		c = data.ptr;
		s = container_get_data(c);
		string_set(s, str);
	} else {
		c = container_alloc(CONTAINER_STRING);
		s = container_get_data(c);
		string_set(s, str);
		return db_store(xfiredb, key, c);
	}

	return -XFIRE_OK;
}

int xfiredb_list_pop(char *key, int *idx, int num)
{
	struct string *s;
	struct container *container;
	struct list_head *lh;
	struct list *c, *tmp;
	db_data_t dbdata;
	int i = 0, rmnum = 0;

	if(db_lookup(xfiredb, key, &dbdata) != -XFIRE_OK)
		return rmnum;

	container = dbdata.ptr;
	lh = container_get_data(container);
	list_for_each_safe(lh, c, tmp) {
		if(idx[i] < 0)
			idx[i] += list_length(lh);

		if(idx[i] == i) {
			list_del(lh, c);
			s = container_of(c, struct string, entry);
			string_destroy(s);
			xfire_free(s);
			rmnum++;
		}

		if(++i >= num)
			break;
	}

	if(list_length(lh) <= 0) {
		if(db_delete(xfiredb, key, &dbdata))
			return rmnum;

		container_destroy(container);
		xfire_free(container);
	}
	return rmnum;
}

int xfiredb_list_get(char *key, char **data, int *idx, int num)
{
	struct string *s;
	struct container *container;
	struct list_head *lh;
	struct list *c;
	char *tmp;
	db_data_t dbdata;
	int i = 0;

	if(db_lookup(xfiredb, key, &dbdata) != -XFIRE_OK)
		return -XFIRE_ERR;

	container = dbdata.ptr;
	lh = container_get_data(container);
	list_for_each(lh, c) {
		if(idx[i] < 0)
			idx[i] += list_length(lh);

		if(idx[i] == i) {
			s = container_of(c, struct string, entry);
			string_get(s, &tmp);
			data[i] = tmp;
		}

		if(++i >= num)
			break;
	}

	return -XFIRE_ERR;
}

int xfiredb_list_set(char *key, int idx, char *data)
{
	struct string *s;
	struct container *container;
	struct list_head *h;
	struct list *c;
	db_data_t dbdata;
	int i, rv = -XFIRE_ERR;

	if(db_lookup(xfiredb, key, &dbdata) != -XFIRE_OK) {
		xfiredb_list_push(key, data, true);
		return -XFIRE_OK;
	}

	container = dbdata.ptr;
	h = container_get_data(container);
	i = 0;

	if(list_length(h) == 0) {
		s = string_alloc(data);
		list_rpush(h, &s->entry);
		return -XFIRE_OK;
	}

	list_for_each(h, c) {
		if(i == idx) {
			s = container_of(c, struct string, entry);
			string_set(s, data);
			rv = -XFIRE_OK;
			break;
		}
		i++;

		if(i >= list_length(h)) {
			s = string_alloc(data);
			list_rpush(h, &s->entry);
			rv = -XFIRE_OK;
			break;
		}
	}

	return rv;
}

int xfiredb_list_push(char *key, char *data, bool left)
{
	struct string *s;
	struct container *c;
	struct list_head *h;
	db_data_t dbdata;
	bool new = false;

	if(db_lookup(xfiredb, key, &dbdata) != -XFIRE_OK) {
		c = container_alloc(CONTAINER_LIST);
		new = true;
	} else {
		c = dbdata.ptr;
	}

	h = container_get_data(c);
	s = string_alloc(data);

	if(left)
		list_lpush(h, &s->entry);
	else
		list_rpush(h, &s->entry);

	if(new)
		db_store(xfiredb, key, c);

	return -XFIRE_OK;
}

int xfiredb_hashmap_get(char *key, char **skey, char **data, int num)
{
	struct string *s;
	struct container *c;
	struct hashmap *hm;
	struct hashmap_node *node;
	char *tmp;
	db_data_t dbdata;
	int i = 0;

	if(db_lookup(xfiredb, key, &dbdata) != -XFIRE_OK)
		return -XFIRE_ERR;

	c = dbdata.ptr;
	hm = container_get_data(c);

	for(; i < num; i++) {
		node = hashmap_find(hm, skey[i]);
		if(!node)
			continue;

		s = container_of(node, struct string, node);
		string_get(s, &tmp);
		data[i] = tmp;
	}

	return -XFIRE_OK;
}

int xfiredb_hashmap_remove(char *key, char **skeys, int num)
{
	struct string *s;
	struct container *c;
	struct hashmap *hm;
	struct hashmap_node *node;
	db_data_t dbdata;
	int i = 0, rmnum = 0;

	if(db_lookup(xfiredb, key, &dbdata) != -XFIRE_OK)
		return rmnum;

	c = dbdata.ptr;
	hm = container_get_data(c);

	for(; i < num; i++) {
		node = hashmap_remove(hm, skeys[i]);
		if(!node)
			continue;
		rmnum++;
		s = container_of(node, struct string, node);
		hashmap_node_destroy(node);
		string_destroy(s);
		xfire_free(s);
	}

	if(!hashmap_size(hm)) {
		if(db_delete(xfiredb, key, &dbdata))
			return rmnum;

		container_destroy(c);
		xfire_free(c);
	}

	return rmnum;
}

int xfiredb_hashmap_add(char *key, char *skey, char *data)
{
	struct string *s;
	struct container *c;
	struct hashmap *hm;
	struct hashmap_node *node;
	bool new = false;
	db_data_t dbdata;

	if(db_lookup(xfiredb, key, &dbdata) != -XFIRE_OK) {
		c = container_alloc(CONTAINER_HASHMAP);
		new = true;
	} else {
		c = dbdata.ptr;
	}

	hm = container_get_data(c);
	node = hashmap_find(hm, skey);
	if(!node) {
		s = string_alloc(data);
		hashmap_add(hm, skey, &s->node);
	} else {
		s = container_of(node, struct string, node);
		string_set(s, data);
	}

	if(new)
		db_store(xfiredb, key, c);

	return -XFIRE_ERR;
}

static void xfiredb_hashmap_free_hook(struct hashmap_node *node)
{
	struct string *s;

	s = container_of(node, struct string, node);
	hashmap_node_destroy(node);
	string_destroy(s);
	xfire_free(s);
}

int xfiredb_key_delete(char *key)
{
	struct list *carriage, *tmp;
	struct list_head *lh;
	struct string *s;
	struct container *c;
	struct hashmap *hm;
	db_data_t data;
	int rv = 0;

	if(db_delete(xfiredb, key, &data) != -XFIRE_OK)
		return rv;

	c = data.ptr;
	switch(c->type) {
	case CONTAINER_STRING:
		container_destroy(c);
		xfire_free(c);
		rv++;
		break;

	case CONTAINER_LIST:
		lh = container_get_data(c);
		list_for_each_safe(lh, carriage, tmp) {
			s = container_of(carriage, struct string, entry);
			string_destroy(s);
			xfire_free(s);
		}

		container_destroy(c);
		xfire_free(c);
		break;

	case CONTAINER_HASHMAP:
		hm = container_get_data(c);
		rv += (int)hashmap_size(hm);
		hashmap_clear(hm, &xfiredb_hashmap_free_hook);
		container_destroy(c);
		xfire_free(c);
		break;

	default:
		rv = -XFIRE_ERR;
		break;
	}

	return rv;
}

