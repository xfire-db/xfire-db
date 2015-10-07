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

#define DISK_CHECK_TABLE \
	"SELECT name FROM sqlite_master WHERE type='table' AND name='xfiredb_data';"

#define DISK_CREATE_TABLE \
	"CREATE TABLE xfiredb_data(" \
	"db_key CHAR(64) PRIMARY KEY NOT NULL," \
	"db_value CHAR(64));"

static int init_hook(void *arg, int argc, char **argv, char **colname)
{
	struct disk *d = arg;

	d->initialised = true;
	return 0;
}

static int disk_create_table(struct disk *disk)
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

	if(disk_create_table(disk) != -XFIRE_OK) {
		fprintf(stderr, "Could not create tables, exiting.\n");
	}

	return disk;
}

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

