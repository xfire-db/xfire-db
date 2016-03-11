/*
 *  Object header
 *  Copyright (C) 2016   Michel Megens <dev@michelmegens.net>
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
 * @addtogroup object
 * @{
 */

#ifndef __HASHMAP__H__
#define __HASHMAP__H__

#include <stdlib.h>
#include <time.h>

#include <xfiredb/xfiredb.h>
#include <xfiredb/types.h>

/**
 * @brief XFireDB base object.
 *
 * Every object needs an assigned key (from the end-user). Then
 * the system assigns it a time stamp (creation time). When the
 * object needs to remove itself after a certain time, the expiration
 * time is also set (end-user supplied). An expiration time of zero means
 * that the object does not expire.
 */
struct object {
	char *key; //!< Object key.

	time_t crea, //!< Creation time.
	       exp; //!< Expire time.
};

#define raw_xfiredb_obj_to(__obj, __type, __field) \
	container_of(__obj, __type, __field)
#define xfiredb_obj_to(__obj, __type) raw_xfiredb_obj_to(__obj, __type, obj)

CDECL
struct object *xfiredb_obj_alloc(const char *key, time_t exp);
void xfiredb_obj_init(const char *key, time_t exp);

void xfiredb_obj_destroy(struct object *obj);
void xfiredb_obj_free(struct object *obj);
CDECL_END

#endif /* __HASHMAP__H__ */

/** @} */

