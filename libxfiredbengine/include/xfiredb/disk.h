/*
 *  Disk I/O header
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

#ifndef __DISK_H__
#define __DISK_H__

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include <xfiredb/xfiredb.h>
#include <xfiredb/types.h>
#include <xfiredb/os.h>
#include <xfiredb/string.h>
#include <xfiredb/list.h>
#include <xfiredb/hashmap.h>
#include <xfiredb/container.h>

#define TABLE_KEY_IDX 0
#define TABLE_SCND_KEY_IDX 1
#define TABLE_TYPE_IDX 2
#define TABLE_DATA_IDX 3

/**
 * @brief Persistent disk structure.
 */
struct disk {
	char *dbpath; //!< Database path.
	void *handle; //!< Database handle.

	time_t last_update; //!< Last update time stamp
	u64 records; //!< Number of saved records.
	bool initialised; //!< Initialisation status.

	xfire_mutex_t lock; //!< Disk lock.
};

CDECL
extern long disk_size(struct disk *d);
extern void disk_clear(struct disk *d);
extern struct disk *disk_create(const char *path);
extern void disk_destroy(struct disk *disk);
extern void disk_dump(struct disk *d, FILE *out);
extern int disk_load(struct disk *disk,
		void (*hook)(int argc, char **rows, char **colnames));

extern int disk_store_list(struct disk *d, char *key, struct list_head *lh);
extern int disk_store_list_entry(struct disk *d, char *key, char *data);
extern int disk_delete_list(struct disk *d, char *key, char *data);
extern int disk_update_list(struct disk *d, char *key, char *data, char *newdata);

extern int disk_store_string(struct disk *d, char *key, char *data);
extern int disk_update_string(struct disk *d, char *key, void *data);
extern int disk_delete_string(struct disk *d, char *key);

extern int disk_store_set_key(struct disk *d, char *key, char *skey);
extern int disk_delete_set_key(struct disk *d, char *key, char *skey);

extern int disk_store_hm(struct disk *d, char *key, struct hashmap *map);
extern int disk_update_hm(struct disk *d, char *key, char *nodekey, char *data);
extern int disk_delete_hashmapnode(struct disk *d, char *key, char *nodekey);
extern int disk_store_hm_node(struct disk *d, char *key, char *nodekey, char *data);

#endif

/** @} */

