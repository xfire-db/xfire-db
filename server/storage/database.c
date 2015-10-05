/*
 *  Database implementation
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
 * @addtogroup database
 * @{
 */

#include <stdlib.h>

#include <xfire/xfire.h>
#include <xfire/types.h>
#include <xfire/mem.h>
#include <xfire/dict.h>
#include <xfire/database.h>

struct database *db_alloc(const char *name)
{
	struct database *db;
	int l;

	db = xfire_zalloc(sizeof(*db));
	db->container = dict_alloc();
	l = strlen(name) + 1;
	db->name = xfire_zalloc(l);

	memcpy(db->name, name, l);
	return db;
}

void db_store(struct database *db, const char *key, void *data)
{
	/*
	 * add the entry to the dictionary.
	 */
	dict_add(db->container, key, data, DICT_PTR);
}

int db_delete(struct database *db, const char *key, db_data_t *data)
{
	union entry_data val;

	dict_delete(db->container, key, &val, false);
	memcpy(data, &val, sizeof(val));

	return -XFIRE_OK;
}

int db_lookup(struct database *db, const char *key, db_data_t *data)
{
	union entry_data val;
	int rv;

	rv = dict_lookup(db->container, key, &val);
	if(rv != -XFIRE_OK)
		return rv;
	else
		memcpy(data, &val, sizeof(val));

	return rv;
}

void db_free(struct database *db)
{
	dict_clear(db->container);
	dict_free(db->container);

	xfire_free(db->name);
	xfire_free(db);
}

/** @} */

