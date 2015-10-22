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

/**
 * @addtogroup engine
 * @{
 */

#include <stdlib.h>

#include <xfire/xfire.h>
#include <xfire/types.h>
#include <xfire/log.h>
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
	char *key, *skey, *data;
	int i;
	bool available;
	db_data_t dbdata;
	struct container *c;
	struct string *s;
	struct list_head *h;
	struct hashmap *map;

	for(i = 0; i < argc; i += 4) {
		type = xfiredb_get_row_type(rows[i + TABLE_TYPE_IDX]);
		key = rows[i + TABLE_KEY_IDX];
		skey = rows[i + TABLE_SCND_KEY_IDX];
		data = rows[i + TABLE_DATA_IDX];
		available = db_lookup(xfiredb, key, &dbdata) == -XFIRE_OK ? true : false;

		switch(type) {
		case CONTAINER_STRING:
			if(available)
				break;

			c = container_alloc(CONTAINER_STRING);
			s = container_get_data(c);
			string_set(s, data);
			db_store(xfiredb, key, c);
			break;

		case CONTAINER_LIST:
			if(available)
				c = dbdata.ptr;
			else
				c = container_alloc(CONTAINER_LIST);

			h = container_get_data(c);
			s = string_alloc(data);
			list_rpush(h, &s->entry);

			if(!available)
				db_store(xfiredb, key, c);
			break;

		case CONTAINER_HASHMAP:
			if(available)
				c = dbdata.ptr;
			else
				c = container_alloc(CONTAINER_HASHMAP);

			map = container_get_data(c);
			s = string_alloc(data);
			hashmap_add(map, skey, &s->node);

			if(!available)
				db_store(xfiredb, key, c);
			break;
		}
	}
}

/**
 * @brief Initialise the XFireDB storage engine.
 */
void xfiredb_init(void)
{
	xfire_log_init(XFIRE_STDOUT, XFIRE_STDERR);
	xfire_log_console(LOG_INIT, "Creating in memory database\n");
	xfiredb = db_alloc("xfiredb");
	xfire_log_console(LOG_INIT, "Initialising storage engine\n");
	xfire_log_console(LOG_INIT, "Initialising background processes\n");
	bg_processes_init();
	xfire_log_console(LOG_INIT, "Initialising background I/O\n");
	bio_init();
	xfire_log_console(LOG_INIT, "Loading data from disk\n");
	disk_load(disk_db, &xfiredb_load_hook);
}

/**
 * @brief Destructor for the XFireDB storage engine.
 */
void xfiredb_exit(void)
{
	bio_sync();
	bio_exit();
	bg_processes_exit();
	db_free(xfiredb);
	xfire_log_exit();
}

/**
 * @brief Get a string.
 * @param key Key to search.
 * @param data Data pointer.
 */
int xfiredb_string_get(char *key, char **data)
{
	struct string *s;
	struct container *c;
	db_data_t dbdata;

	if(db_lookup(xfiredb, key, &dbdata) != -XFIRE_OK) {
		*data = NULL;
		return -XFIRE_ERR;
	}

	c = dbdata.ptr;
	if(!container_check_type(c, CONTAINER_STRING))
		return -XFIRE_ERR;

	s = container_get_data(c);
	string_get(s, data);
	return -XFIRE_OK;
}

/**
 * @brief Set the data of a string.
 * @param key Key to store under.
 * @param str Data to set.
 * @return An error code.
 */
int xfiredb_string_set(char *key, char *str)
{
	struct string *s;
	struct container *c;
	db_data_t data;
	char *bio_key, *bio_str;
	int rv = -XFIRE_OK;
	bio_operation_t op;

	xfire_sprintf(&bio_key, "%s", key);
	xfire_sprintf(&bio_str, "%s", str);
	if(!db_lookup(xfiredb, key, &data)) {
		c = data.ptr;
		if(!container_check_type(c, CONTAINER_STRING)) {
			xfire_free(bio_key);
			xfire_free(bio_str);
			return -XFIRE_ERR;
		}

		s = container_get_data(c);
		string_set(s, str);
		op = STRING_UPDATE;
	} else {
		op = STRING_ADD;
		c = container_alloc(CONTAINER_STRING);
		s = container_get_data(c);
		string_set(s, str);
		rv = db_store(xfiredb, key, c);
		if(rv) {
			container_destroy(c);
			return rv;
		}
	}

	bio_queue_add(bio_key, NULL, bio_str, op);
	return rv;
}

/**
 * @brief Get the length of a list.
 * @param key List key.
 * @return Length of the list under \p key.
 */
int xfiredb_list_length(char *key)
{
	struct container *c;
	struct list_head *h;
	db_data_t dbdata;

	if(db_lookup(xfiredb, key, &dbdata) != -XFIRE_OK)
		return -XFIRE_ERR;

	c = dbdata.ptr;
	if(!container_check_type(c, CONTAINER_LIST))
		return -XFIRE_ERR;

	h = container_get_data(c);
	return list_length(h);
}

/**
 * @brief Pop a list entry.
 * @param key List key.
 * @param idx Index array.
 * @param num Number of indexes in \p idx.
 * @return An error code.
 */
int xfiredb_list_pop(char *key, int *idx, int num)
{
	struct string *s;
	struct container *container;
	struct list_head *lh;
	struct list *c, *tmp;
	char *bio_key, *bio_data;
	db_data_t dbdata;
	int i = 0, counter = 0;

	if(db_lookup(xfiredb, key, &dbdata) != -XFIRE_OK)
		return counter;

	container = dbdata.ptr;
	if(!container_check_type(container, CONTAINER_LIST))
		return -XFIRE_ERR;

	lh = container_get_data(container);
	list_for_each_safe(lh, c, tmp) {
		if(idx[counter] < 0)
			idx[counter] += list_length(lh);

		if(idx[counter] == i) {
			list_del(lh, c);
			s = container_of(c, struct string, entry);
			xfire_sprintf(&bio_key, "%s", key);
			string_get(s, &bio_data);
			bio_queue_add(bio_key, bio_data, NULL, LIST_DEL);
			string_destroy(s);
			xfire_free(s);
			counter++;
		}

		if(counter >= num)
			break;
		i++;
	}

	if(list_length(lh) <= 0) {
		if(db_delete(xfiredb, key, &dbdata))
			return counter;

		container_destroy(container);
		xfire_free(container);
	}

	return counter;
}

/**
 * @brief Get a number of list elements.
 * @param key List key.
 * @param data Data storage pointer.
 * @param idx Indexes to lookup.
 * @param num Number of indexes to lookup.
 * @note Both the \p data and \p idx array have to be able to hold
 * \p num entry's.
 */
int xfiredb_list_get(char *key, char **data, int *idx, int num)
{
	struct string *s;
	struct container *container;
	struct list_head *lh;
	struct list *c;
	char *tmp;
	db_data_t dbdata;
	int i = 0, counter = 0;

	if(db_lookup(xfiredb, key, &dbdata) != -XFIRE_OK)
		return -XFIRE_ERR;

	container = dbdata.ptr;
	if(!container_check_type(container, CONTAINER_LIST))
		return -XFIRE_ERR;

	lh = container_get_data(container);
	list_for_each(lh, c) {
		if(idx[counter] < 0)
			idx[counter] += list_length(lh);

		if(idx[counter] == i) {
			s = container_of(c, struct string, entry);
			string_get(s, &tmp);
			data[counter] = tmp;
			counter++;
		}

		if(counter >= num)
			break;
		i++;
	}

	return -XFIRE_OK;
}

/**
 * @brief Set a list entry's data.
 * @param key List key.
 * @param idx List index to set.
 * @param data Data to set.
 * @note If the index \p idx doesn't exist the data
 * will be appended to the list as a new entry.
 */
int xfiredb_list_set(char *key, int idx, char *data)
{
	struct string *s;
	struct container *container;
	struct list_head *h;
	struct list *c;
	char *bio_key, *bio_data, *bio_newdata;
	db_data_t dbdata;
	int i, rv = -XFIRE_ERR;

	if(db_lookup(xfiredb, key, &dbdata) != -XFIRE_OK) {
		return xfiredb_list_push(key, data, true);
	}

	container = dbdata.ptr;
	if(!container_check_type(container, CONTAINER_LIST))
		return -XFIRE_ERR;

	h = container_get_data(container);
	i = 0;

	xfire_sprintf(&bio_key, "%s", key);
	xfire_sprintf(&bio_newdata, "%s", data);
	if(list_length(h) == 0) {
		s = string_alloc(data);
		list_rpush(h, &s->entry);
		bio_queue_add(bio_key, NULL, bio_newdata, LIST_ADD);
		return -XFIRE_OK;
	}

	list_for_each(h, c) {
		if(i == idx) {
			s = container_of(c, struct string, entry);
			string_get(s, &bio_data);
			bio_queue_add(bio_key, bio_data, bio_newdata, LIST_UPDATE);
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

/**
 * @brief Push a new list entry.
 * @param key List key.
 * @param data Data to push.
 * @param left Set to true if \p data should be pushed at
 * the start, false for the end.
 * @return An error code.
 */
int xfiredb_list_push(char *key, char *data, bool left)
{
	struct string *s;
	struct container *c;
	struct list_head *h;
	char *bio_key, *bio_data;
	db_data_t dbdata;
	bool new = false;

	if(db_lookup(xfiredb, key, &dbdata) != -XFIRE_OK) {
		c = container_alloc(CONTAINER_LIST);
		new = true;
	} else {
		c = dbdata.ptr;
		if(!container_check_type(c, CONTAINER_LIST))
			return -XFIRE_ERR;
	}

	h = container_get_data(c);
	s = string_alloc(data);
	xfire_sprintf(&bio_data, "%s", data);
	xfire_sprintf(&bio_key, "%s", key);
	bio_queue_add(bio_key, NULL, bio_data, LIST_ADD);

	if(left)
		list_lpush(h, &s->entry);
	else
		list_rpush(h, &s->entry);

	if(new)
		db_store(xfiredb, key, c);

	return -XFIRE_OK;
}

/**
 * @brief Get a hashmap entry.
 * @param key Hashmap key
 * @param skey Array of hashmap keys.
 * @param data Data storage array.
 * @param num Number of entry's is \p skey and \p data.
 */
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
	if(!container_check_type(c, CONTAINER_HASHMAP))
		return -XFIRE_ERR;

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

/**
 * @brief Remove a hashmap node.
 * @param key Hashmap key.
 * @param skeys Array of hashmap key's.
 * @param num Length of the \p skey array.
 * @return An error code.
 */
int xfiredb_hashmap_remove(char *key, char **skeys, int num)
{
	struct string *s;
	struct container *c;
	struct hashmap *hm;
	struct hashmap_node *node;
	char *bio_key, *bio_skey;
	db_data_t dbdata;
	int i = 0, rmnum = 0;

	if(db_lookup(xfiredb, key, &dbdata) != -XFIRE_OK)
		return rmnum;

	c = dbdata.ptr;
	if(!container_check_type(c, CONTAINER_HASHMAP))
		return -XFIRE_ERR;

	hm = container_get_data(c);

	for(; i < num; i++) {
		node = hashmap_remove(hm, skeys[i]);
		if(!node)
			continue;
		rmnum++;
		xfire_sprintf(&bio_key, "%s", key);
		xfire_sprintf(&bio_skey, "%s", skeys[i]);
		bio_queue_add(bio_key, bio_skey, NULL, HM_DEL);
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

/**
 * @brief Set the value of a hashmap node.
 * @param key Hashmap key.
 * @param skey Key within the hashmap (key to set).
 * @param data Data to set.
 * @note If the \p skey key doesn't exist, it is created and
 * added to the hashmap.
 */
int xfiredb_hashmap_set(char *key, char *skey, char *data)
{
	struct string *s;
	struct container *c;
	struct hashmap *hm;
	struct hashmap_node *node;
	char *bio_key, *bio_skey, *bio_data;
	bool new = false;
	db_data_t dbdata;

	if(db_lookup(xfiredb, key, &dbdata) != -XFIRE_OK) {
		c = container_alloc(CONTAINER_HASHMAP);
		new = true;
	} else {
		c = dbdata.ptr;
		if(!container_check_type(c, CONTAINER_HASHMAP))
			return -XFIRE_ERR;
	}

	hm = container_get_data(c);
	node = hashmap_find(hm, skey);
	xfire_sprintf(&bio_key, "%s", key);
	xfire_sprintf(&bio_skey, "%s", skey);
	xfire_sprintf(&bio_data, "%s", data);

	if(!node) {
		s = string_alloc(data);
		hashmap_add(hm, skey, &s->node);
		bio_queue_add(bio_key, bio_skey, bio_data, HM_ADD);
	} else {
		s = container_of(node, struct string, node);
		string_set(s, data);
		bio_queue_add(bio_key, bio_skey, bio_data, HM_UPDATE);
	}

	if(new)
		db_store(xfiredb, key, c);

	return -XFIRE_OK;
}

/**
 * @brief Delete a key from the database.
 * @param key Key to delete.
 * @return An error code.
 */
int xfiredb_key_delete(char *key)
{
	auto void xfiredb_hashmap_free_hook(struct hashmap_node *node);
	struct list *carriage, *tmp;
	struct list_head *lh;
	struct string *s;
	struct container *c;
	struct hashmap *hm;
	char *bio_key, *bio_data;
	db_data_t data;
	int rv = 0;

	if(db_delete(xfiredb, key, &data) != -XFIRE_OK)
		return rv;

	c = data.ptr;
	switch(c->type) {
	case CONTAINER_STRING:
		xfire_sprintf(&bio_key, "%s", key);
		bio_queue_add(bio_key, NULL, NULL, STRING_DEL);
		container_destroy(c);
		xfire_free(c);
		rv++;
		break;

	case CONTAINER_LIST:
		xfire_sprintf(&bio_key, "%s", key);
		lh = container_get_data(c);
		list_for_each_safe(lh, carriage, tmp) {
			s = container_of(carriage, struct string, entry);
			string_get(s, &bio_data);
			bio_queue_add(bio_key, bio_data, NULL, LIST_DEL);
			list_del(lh, carriage);
			string_destroy(s);
			xfire_free(s);
			rv++;
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

	void xfiredb_hashmap_free_hook(struct hashmap_node *node)
	{
		struct string *str;
		char *bkey, *bskey;

		str = container_of(node, struct string, node);
		xfire_sprintf(&bkey, "%s", key);
		xfire_sprintf(&bskey, "%s", node->key);
		bio_queue_add(bkey, bskey, NULL, HM_DEL);
		hashmap_node_destroy(node);
		string_destroy(str);
		xfire_free(str);
	}
}

/** @} */

