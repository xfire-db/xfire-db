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

#ifndef __DISK_H__
#define __DISK_H__

#include <stdlib.h>
#include <time.h>

#include <xfire/xfire.h>
#include <xfire/types.h>
#include <xfire/os.h>

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
extern struct disk *disk_create(const char *path);
extern void disk_destroy(struct disk *disk);

extern int disk_store(struct disk *disk, char *key, void *data, size_t length);
extern int disk_update(struct disk *disk, char *key, void *data, size_t length);

#endif

