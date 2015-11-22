/*
 *  Disk I/O
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
 * @addtogroup disk
 * @{
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sqlite3.h>
#include <stdio.h>

#include <xfiredb/xfiredb.h>
#include <xfiredb/types.h>
#include <xfiredb/log.h>
#include <xfiredb/disk.h>
#include <xfiredb/mem.h>
#include <xfiredb/error.h>
#include <xfiredb/container.h>
#include <xfiredb/string.h>
#include <xfiredb/list.h>
#include <xfiredb/hashmap.h>

#define DISK_CHECK_TABLE \
	"SELECT name FROM sqlite_master WHERE type='table' AND name='xfiredb_data';"

#define DISK_CREATE_TABLE \
	"CREATE TABLE xfiredb_data(" \
	"db_key CHAR(64) NOT NULL, " \
	"db_secondary_key, " \
	"db_type CHAR(64), " \
	"db_value BLOB);"

static int dummy_hook(void *arg, int argc, char **argv, char **colname)
{
	return 0;
}

static int init_hook(void *arg, int argc, char **argv, char **colname)
{
	struct disk *d = arg;

	d->initialised = true;
	return 0;
}

static int disk_create_main_table(struct disk *disk)
{
	int rc;
	char *errmsg;

	rc = sqlite3_exec(disk->handle, DISK_CHECK_TABLE, &init_hook, disk, &errmsg);
	if(rc != SQLITE_OK)
		xfire_log_console(LOG_DISK, "Error occured while creating tables: %s\n", errmsg);

	if(!disk->initialised) {
		/* table's non existent, creating them */
		rc = sqlite3_exec(disk->handle, DISK_CREATE_TABLE, &init_hook, disk, &errmsg);
		if(rc != SQLITE_OK)
			xfire_log_console(LOG_DISK, "Error occured while creating tables: %s\n", errmsg);
	}

	return -XFIRE_OK;
}

/**
 * @brief Create a new persistent disk.
 * @param path Path to the disk.
 * @return Disk data structure.
 */
struct disk *disk_create(const char *path)
{
	sqlite3 *db;
	struct disk *disk;
	int rc;
	int len;

	rc = sqlite3_open(path, &db);

	if(rc) {
		fprintf(stderr, "Cannot create disk file %s: %s\n",
				path, sqlite3_errmsg(db));
		exit(-EXIT_FAILURE);
	}

	disk = xfire_zalloc(sizeof(*disk));
	len = strlen(path);

	disk->dbpath = xfire_zalloc(len + 1);
	memcpy(disk->dbpath, path, len);
	disk->handle = db;
	disk->records = 0ULL;
	xfire_mutex_init(&disk->lock);

	if(disk_create_main_table(disk) != -XFIRE_OK)
		fprintf(stderr, "Could not create tables, exiting.\n");

	return disk;
}

#define DISK_STORE_QUERY \
	"INSERT INTO xfiredb_data (db_key, db_secondary_key, db_type, db_value) " \
	"VALUES ('%s', '%s', '%s', '%s');"
#define DISK_SELECT_QUERY \
	"SELECT * FROM xfiredb_data WHERE db_key = '%s';"

#define DISK_CLEAR_QUERY \
	"DELETE * FROM xfiredb_data;"

void disk_clear(struct disk *d)
{
	int rc;
	char *msg, *query;

	xfire_sprintf(&query, DISK_CLEAR_QUERY);
	rc = sqlite3_exec(d->handle, query, &dummy_hook, NULL, &msg);

	if(rc != SQLITE_OK)
		xfire_log_err(LOG_DISK, "Disk clear failed: %s\n", msg);

	sqlite3_free(msg);
	xfire_free(query);
}

struct hm_store_data {
	char *key;
	struct disk *d;
};

/**
 * @brief Store a hashmap node.
 * @param d Disk to store onto.
 * @param key Key to store the node under.
 * @param nodekey Key of \p data within the hashmap.
 * @param data Data to store.
 * @return An error code.
 */
int disk_store_hm_node(struct disk *d, char *key, char *nodekey, char *data)
{
	int rc;
	char *msg, *query;

	xfire_sprintf(&query, DISK_STORE_QUERY, key, nodekey, "hashmap", data);
	rc = sqlite3_exec(d->handle, query, &dummy_hook, NULL, &msg);

	if(rc != SQLITE_OK)
		fprintf(stderr, "Disk store failed: %s\n", msg);

	sqlite3_free(msg);
	xfire_free(query);
	
	return rc == SQLITE_OK ? -XFIRE_OK : -XFIRE_ERR;
}

/**
 * @brief Store an entire hashmap.
 * @param d Disk to store \p map on.
 * @param key Key to store \p map under.
 * @param map Hashmap to store.
 * @return An error code.
 */
int disk_store_hm(struct disk *d, char *key, struct hashmap *map)
{
	struct hashmap_node *node;
	char *data, *msg, *query;
	struct string *s;
	struct hashmap_iterator *it;
	int rc;

	it = hashmap_new_iterator(map);
	for(node = hashmap_iterator_next(it); node;
			node = hashmap_iterator_next(it)) {
		s = container_of(node, struct string, node);
		string_get(s, &data);
		xfire_sprintf(&query, DISK_STORE_QUERY, key, node->key, "hashmap", data);
		rc = sqlite3_exec(d->handle, query, &dummy_hook, d, &msg);

		if(rc != SQLITE_OK)
			fprintf(stderr, "Disk store failed: %s\n", msg);

		sqlite3_free(msg);
		xfire_free(query);
		xfire_free(data);
	}

	return -XFIRE_OK;
}

/**
 * @brief Store a set key.
 * @param d Disk to store onto.
 * @param key Key to store the key under.
 * @param skey Set key to store.
 * @return An error code.
 */
int disk_store_set_key(struct disk *d, char *key, char *skey)
{
	int rc;
	char *msg, *query;

	xfire_sprintf(&query, DISK_STORE_QUERY, key, skey, "set", "null");
	rc = sqlite3_exec(d->handle, query, &dummy_hook, NULL, &msg);

	if(rc != SQLITE_OK)
		fprintf(stderr, "Disk store failed: %s\n", msg);

	sqlite3_free(msg);
	xfire_free(query);
	
	return rc == SQLITE_OK ? -XFIRE_OK : -XFIRE_ERR;
}

/**
 * @brief Store a list entry.
 * @param d Disk to store on.
 * @param key Key to store \p data under.
 * @param data Data to store.
 * @return An error code.
 */
int disk_store_list_entry(struct disk *d, char *key, char *data)
{
	int rc;
	char *msg, *query;

	xfire_sprintf(&query, DISK_STORE_QUERY, key, "null", "list", data);
	rc = sqlite3_exec(d->handle, query, &dummy_hook, d, &msg);

	if(rc != SQLITE_OK)
		fprintf(stderr, "Disk store failed: %s\n", msg);

	xfire_free(query);
	sqlite3_free(msg);

	return (rc == SQLITE_OK) ? -XFIRE_OK : -XFIRE_ERR;
}

/**
 * @brief Store a string list.
 * @param d Disk to store to.
 * @param key Key to store the list under.
 * @param lh List head to store.
 * @return An error code.
 */
int disk_store_list(struct disk *d, char *key, struct list_head *lh)
{
	struct list *c;
	struct string *s;
	char *data;

	list_for_each(lh, c) {
		s = container_of(c, struct string, entry);
		string_get(s, &data);
		if(disk_store_list_entry(d, key, data)) {
			xfire_free(data);
			return -XFIRE_ERR;
		}

		xfire_free(data);
	}

	return -XFIRE_OK;
}

/**
 * @brief Store a key-value pair on the disk.
 * @param d Disk to store on.
 * @param key Key to store.
 * @param data Data to store (under \p key).
 * @return Error code.
 */
int disk_store_string(struct disk *d, char *key, char *data)
{
	int rc;
	char *msg, *query;

	xfire_sprintf(&query, DISK_STORE_QUERY, key, "null", "string", data);
	rc = sqlite3_exec(d->handle, query, &dummy_hook, d, &msg);

	if(rc != SQLITE_OK) {
		fprintf(stderr, "Disk store failed: %s\n", msg);
		sqlite3_free(msg);
		xfire_free(query);
		return -XFIRE_ERR;
	}

	sqlite3_free(msg);
	xfire_free(query);
	return rc;
}

static int dump_hook(void *arg, int argc, char **row, char **colname)
{
	int i;
	FILE *output = arg;

	for(i = 0; i < argc; i+=4)
		fprintf(output, "%s = %s :: %s = %s :: %s = %s :: %s = %s\n", colname[i], row[i],
				colname[i+1], row[i+1], colname[i+2], row[i+2], colname[i+3], row[i+3]);

	//len = strlen(row[1]);
	//*data = xfire_zalloc(len + 1);
	//memcpy(*data, row[1], len);

	return 0;
}

/**
 * @brief Dump the disk.
 * @param d Disk to dump.
 * @param output Output stream to dump the disk to.
 * @note Function for debugging purposes only.
 */
void disk_dump(struct disk *d, FILE *output)
{
	int rc;
	char *msg;

	rc = sqlite3_exec(d->handle, "SELECT * FROM xfiredb_data", &dump_hook, output, &msg);

	switch(rc) {
	case SQLITE_OK:
		break;

	default:
		fprintf(stderr, "Disk dump failed: %s\n", msg);
		sqlite3_free(msg);
		return;
	}

	sqlite3_free(msg);
}

#define DISK_UPDATE_HM_QUERY \
	"UPDATE xfiredb_data " \
	"SET db_value = '%s' " \
	"WHERE db_key = '%s' AND db_secondary_key = '%s';"

/**
 * @brief Update hashmap entry.
 * @param d Disk to update.
 * @param key Key to update.
 * @param nodekey Node key to update.
 * @param data Data to set.
 * @return An error code.
 */
int disk_update_hm(struct disk *d, char *key, char *nodekey, char *data)
{
	int rc;
	char *msg, *query;

	xfire_sprintf(&query, DISK_UPDATE_HM_QUERY, data, key, nodekey);
	rc = sqlite3_exec(d->handle, query, &dummy_hook, NULL, &msg);

	if(rc != SQLITE_OK)
		fprintf(stderr, "Disk update failed: %s\n", msg); 

	xfire_free(query);
	sqlite3_free(msg);

	return (rc == SQLITE_OK) ? -XFIRE_OK : -XFIRE_ERR;
}

#define DISK_UPDATE_LIST_QUERY \
	"UPDATE xfiredb_data SET db_value = '%s' " \
	"WHERE ROWID IN (SELECT ROWID FROM xfiredb_data WHERE " \
	"db_key = '%s' AND db_value = '%s' AND db_type = 'list' LIMIT 1);"

/**
 * @brief Update a list entry.
 * @param d Disk to update.
 * @param key Key to update.
 * @param data Data currently stored under \p key.
 * @param newdata Data to set.
 * @return An error code.
 */
int disk_update_list(struct disk *d, char *key, char *data, char *newdata)
{
	int rc;
	char *msg, *query;

	xfire_sprintf(&query, DISK_UPDATE_LIST_QUERY, newdata, key, data);
	rc = sqlite3_exec(d->handle, query, &dummy_hook, d, &msg);

	if(rc != SQLITE_OK)
		fprintf(stderr, "Disk update failed: %s\n", msg);

	sqlite3_free(msg);
	xfire_free(query);
	return rc == SQLITE_OK ? -XFIRE_OK : -XFIRE_ERR;
}

#define DISK_UPDATE_STRING_QUERY \
	"UPDATE xfiredb_data SET db_value = '%s' " \
	"WHERE db_key = '%s' AND db_type = 'string';"

/**
 * @brief Update a key-value pair.
 * @param d Disk to search on.
 * @param key Key to update.
 * @param data New data to set under \p key.
 * @return Error code.
 */
int disk_update_string(struct disk *d, char *key, void *data)
{
	int rc;
	char *msg, *query;

	xfire_sprintf(&query, DISK_UPDATE_STRING_QUERY, data, key);
	rc = sqlite3_exec(d->handle, query, &dummy_hook, d, &msg);

	if(rc != SQLITE_OK)
		fprintf(stderr, "Disk update failed: %s\n", msg);

	sqlite3_free(msg);
	xfire_free(query);
	return rc == SQLITE_OK ? -XFIRE_OK : -XFIRE_ERR;
}

#define DISK_DELETE_STRING_QUERY \
	"DELETE FROM xfiredb_data " \
	"WHERE db_type = 'string' AND db_key = '%s';"
#define DISK_DELETE_LIST_QUERY \
	"DELETE FROM xfiredb_data WHERE ROWID in (" \
	"SELECT ROWID FROM xfiredb_data WHERE db_key = '%s' AND " \
	"db_value = '%s' AND db_type = 'list' LIMIT 1);"

#define DISK_DELETE_HM_QUERY \
	"DELETE FROM xfiredb_data " \
	"WHERE db_type = 'hashmap' AND db_key = '%s' AND db_secondary_key = '%s';"

#define DISK_DELETE_SET_QUERY \
	"DELETE FROM xfiredb_data " \
	"WHERE db_type = 'set' AND db_key = '%s' AND db_secondary_key = '%s';"
/**
 * @brief Delete a hashmap node.
 * @param d Disk to delete from.
 * @param key Key to delete.
 * @param nodekey Hashmap key to delete.
 */
int disk_delete_hashmapnode(struct disk *d, char *key, char *nodekey)
{
	int rc;
	char *msg, *query;

	xfire_sprintf(&query, DISK_DELETE_HM_QUERY, key, nodekey);
	rc = sqlite3_exec(d->handle, query, &dummy_hook, d, &msg);

	if(rc != SQLITE_OK)
		fprintf(stderr, "Disk delete failed: %s\n", msg);

	sqlite3_free(msg);
	xfire_free(query);
	return rc == SQLITE_OK ? -XFIRE_OK : -XFIRE_ERR;
}

/**
 * @brief Delete a set key.
 * @param d Disk to delete from.
 * @param key Key to delete.
 * @param skey Set key to delete.
 */
int disk_delete_set_key(struct disk *d, char *key, char *skey)
{
	int rc;
	char *msg, *query;

	xfire_sprintf(&query, DISK_DELETE_SET_QUERY, key, skey);
	rc = sqlite3_exec(d->handle, query, &dummy_hook, d, &msg);

	if(rc != SQLITE_OK)
		fprintf(stderr, "Disk delete failed: %s\n", msg);

	sqlite3_free(msg);
	xfire_free(query);
	return rc == SQLITE_OK ? -XFIRE_OK : -XFIRE_ERR;
}

/**
 * @brief Delete a list entry from disk.
 * @param d Disk to delete from.
 * @param key Key to delete.
 * @param data Data currently stored on the disk under this entry.
 */
int disk_delete_list(struct disk *d, char *key, char *data)
{
	int rc;
	char *msg, *query;

	xfire_sprintf(&query, DISK_DELETE_LIST_QUERY, key, data);
	rc = sqlite3_exec(d->handle, query, &dummy_hook, d, &msg);

	if(rc != SQLITE_OK)
		fprintf(stderr, "Disk delete failed: %s\n", msg);

	sqlite3_free(msg);
	xfire_free(query);
	return rc == SQLITE_OK ? -XFIRE_OK : -XFIRE_ERR;
}

/**
 * @brief Delete a string.
 * @param d Disk to delete from.
 * @param key Key to delete.
 */
int disk_delete_string(struct disk *d, char *key)
{
	int rc;
	char *msg, *query;

	xfire_sprintf(&query, DISK_DELETE_STRING_QUERY, key);
	rc = sqlite3_exec(d->handle, query, &dummy_hook, d, &msg);

	if(rc != SQLITE_OK)
		fprintf(stderr, "Disk delete failed: %s\n", msg);

	sqlite3_free(msg);
	xfire_free(query);
	return rc == SQLITE_OK ? -XFIRE_OK : -XFIRE_ERR;
}

static int disk_load_hook(void *arg, int argc, char **rows, char **colname)
{
	void (*hook)(int argc, char **rows, char **colnames) = arg;

	hook(argc, rows, colname);
	return 0;
}

static int disk_size_hook(void *arg, int argc, char **rows, char **colname)
{
	long *size = arg;

	*size = argc;
	return 0;
}

long disk_size(struct disk *d)
{
	int rc;
	char *msg;
	long size;

	rc = sqlite3_exec(d->handle, "SELECT * FROM xfiredb_data", &disk_size_hook, &size, &msg);

	switch(rc) {
	case SQLITE_OK:
		break;

	default:
		fprintf(stderr, "Disk load failed: %s\n", msg);
		sqlite3_free(msg);
		return -XFIRE_ERR;
	}

	sqlite3_free(msg);
	return size;
}

#define DISK_LOAD_QUERY "SELECT * FROM xfiredb_data WHERE db_key='%s'"

int disk_load_key(struct disk *d, char *key, void (*hook)(int argc, char **rows, char **colnames))
{
	int rc;
	char *msg, *query;

	xfire_sprintf(&query, DISK_LOAD_QUERY, key);
	rc = sqlite3_exec(d->handle, query, &disk_load_hook, hook, &msg);

	switch(rc) {
	case SQLITE_OK:
		break;

	default:
		fprintf(stderr, "Disk load failed: %s\n", msg);
		sqlite3_free(msg);
		return -XFIRE_ERR;
	}

	sqlite3_free(msg);
	return -XFIRE_ERR;
}

/**
 * @brief Load the disk into memory.
 * @param d Disk to load.
 * @param hook Load hook.
 * @return Error code.
 */
int disk_load(struct disk *d, void (*hook)(int argc, char **rows, char **colnames))
{
	int rc;
	char *msg;

	rc = sqlite3_exec(d->handle, "SELECT * FROM xfiredb_data", &disk_load_hook, hook, &msg);

	switch(rc) {
	case SQLITE_OK:
		break;

	default:
		fprintf(stderr, "Disk load failed: %s\n", msg);
		sqlite3_free(msg);
		return -XFIRE_ERR;
	}

	sqlite3_free(msg);
	return -XFIRE_ERR;
}

/**
 * @brief Destroy a disk data structure.
 * @param disk Disk to destroy.
 */
void disk_destroy(struct disk *disk)
{
	sqlite3 *db;

	if(!disk || !disk->handle)
		return;

	db = disk->handle;
	sqlite3_close(db);

	xfire_mutex_destroy(&disk->lock);
	xfire_free(disk->dbpath);
	xfire_free(disk);
}

/** @} */

