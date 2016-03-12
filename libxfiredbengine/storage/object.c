/*
 *  Base object
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

#include <stdlib.h>
#include <time.h>

#include <xfiredb/xfiredb.h>
#include <xfiredb/types.h>
#include <xfiredb/error.h>
#include <xfiredb/object.h>
#include <xfiredb/time.h>
#include <xfiredb/mem.h>
#include <xfiredb/bitops.h>

struct object *object_alloc(void)
{
	struct object *obj;

	obj = xfiredb_zalloc(sizeof(*obj));
	object_init(obj);
	return obj;
}

void object_init(struct object *obj)
{
	if(!obj)
		return;

	obj->crea = xfiredb_time_stamp();
	xfiredb_spinlock_init(&obj->lock);
	obj->exp = 0;
	obj->flags = 0UL;
}

bool object_has_expired(struct object *obj)
{
	time_t now;
	bool rv = false;

	if(!obj)
		return false;

	if(__test_bit(OBJECT_EXPIRED_FLAG, &obj->flags))
		return true;

	now = xfiredb_time_stamp();
	object_lock(obj);
	rv = time_after(now, obj->exp) ? true : false;

	if(rv)
		__set_bit(OBJECT_EXPIRED_FLAG, &obj->flags);
	object_unlock(obj);

	return rv;
}

int object_set_expiry(struct object *obj, time_t exp)
{
	time_t now;

	if(!obj || !exp)
		return -XFIREDB_ERR;

	now = xfiredb_time_stamp();
	if(time_after(now, exp))
		return -XFIREDB_ERR; /* already expired, pointless */

	object_lock(obj);
	__clear_bit(OBJECT_EXPIRED_FLAG, &obj->flags);
	obj->exp = exp;
	object_unlock(obj);

	return -XFIREDB_OK;
}

void object_destroy(struct object *obj)
{
	xfiredb_spinlock_destroy(&obj->lock);
}

void object_free(struct object *obj)
{
	if(!obj)
		return;

	object_destroy(obj);
	xfiredb_free(obj);
}

/** @} */

