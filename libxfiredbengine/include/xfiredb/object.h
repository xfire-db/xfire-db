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
#include <xfiredb/os.h>

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
	time_t crea, //!< Creation time.
	       exp; //!< Expire time.

	unsigned long flags; //!< Object flags
	xfiredb_spinlock_t lock; //!< Object lock.
};

/**
 * @name Object flags
 * @{
 */
#define OBJECT_INTREE_FLAG          0 //!< Object in-tree flag.
#define OBJECT_EXPIRED_FLAG         1 //!< Object expired flag.
/** @} */

CDECL
extern struct object *object_alloc(void);
extern void object_init(struct object *obj);

extern bool object_has_expired(struct object *obj);
extern int object_set_expiry(struct object *obj, time_t exp);

extern void object_destroy(struct object *obj);
extern void object_free(struct object *obj);

/**
 * @brief Lock a given object.
 * @param obj Object to lock.
 */
static inline void object_lock(struct object *obj)
{
	if(!obj)
		return;

	xfiredb_spin_lock(&obj->lock);
}

/**
 * @brief Unlock an object.
 * @param obj Object to unlock.
 */
static inline void object_unlock(struct object *obj)
{
	if(!obj)
		return;

	xfiredb_spin_unlock(&obj->lock);
}

CDECL_END

#endif /* __HASHMAP__H__ */

/** @} */

