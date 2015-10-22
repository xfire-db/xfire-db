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

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sqlite3.h>
#include <stdio.h>

#include <xfire/xfire.h>
#include <xfire/types.h>
#include <xfire/disk.h>
#include <xfire/mem.h>
#include <xfire/error.h>
#include <xfire/container.h>
#include <xfire/string.h>
#include <xfire/list.h>
#include <xfire/hashmap.h>

/**
 * @addtogroup disk
 * @{
 */

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
		fprintf(stderr, "Error occured while creating tables: %s\n", errmsg);

	if(!disk->initialised) {
		/* table's non existent, creating them */
		rc = sqlite3_exec(disk->handle, DISK_CREATE_TABLE, &init_hook, disk, &errmsg);
		if(rc != SQLITE_OK)
			fprintf(stderr, "Error occured while creating tables: %s\n", errmsg);
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
	} else {
		fprintf(stdout, "Database created: %s\n", path);
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

struct hm_store_data {
	char *key;
	struct disk *d;
};

static void disk_hm_iterate(struct hashmap *map, struct hashmap_node *node)
{
	int rc;
	char *data, *msg, *query;
	struct hm_store_data *priv = map->privdata;
	struct string *s = container_of(node, struct string, node);

	string_get(s, &data);
	xfire_sprintf(&query, DISK_STORE_QUERY, priv->key, node->key, "hashmap", data);
	rc = sqlite3_exec(priv->d->handle, query, &dummy_hook, priv->d, &msg);

	if(rc != SQLITE_OK)
		fprintf(stderr, "Disk store failed: %s\n", msg);

	sqlite3_free(msg);
	xfire_free(query);
	xfire_free(data);
}

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

int disk_store_hm(struct disk *d, char *key, struct hashmap *map)
{
	struct hm_store_data data;

	data.key = key;
	data.d = d;
	map->privdata = &data;
	hashmap_iterate(map, &disk_hm_iterate);
	map->privdata = NULL;

	return -XFIRE_OK;
}

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
 * @param length Length of data.
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

	for(i = 0; i < argc; i+=4)
		printf("%s = %s :: %s = %s :: %s = %s :: %s = %s\n", row[i], colname[i],
				row[i+1], colname[i+1], row[i+2], colname[i+2], row[i+3], colname[i+3]);

	//len = strlen(row[1]);
	//*data = xfire_zalloc(len + 1);
	//memcpy(*data, row[1], len);

	return 0;
}

/**
 * @brief Free a lookup result.
 * @param x Result to free.
 * @see disk_lookup
 *
 * Used to free a result returned by disk_lookup.
 */
void disk_result_free(void *x)
{
	if(!x)
		return;

	xfire_free(x);
}

void disk_dump(struct disk *d)
{
	int rc;
	char *msg;

	rc = sqlite3_exec(d->handle, "SELECT * FROM xfiredb_data", &dump_hook, NULL, &msg);

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
 * @param length Length of \p data.
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
