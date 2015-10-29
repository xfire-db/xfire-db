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

#include <xfiredb/xfiredb.h>
#include <xfiredb/types.h>
#include <xfiredb/mem.h>
#include <xfiredb/dict.h>
#include <xfiredb/database.h>
#include <xfiredb/container.h>

/**
 * @brief Allocate a new database.
 * @param name Database name.
 * @return The allocated database.
 */
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

/**
 * @brief Update a database key.
 * @param db Database to look in for \p key.
 * @param key Key which has to be updated.
 * @param c New data to set.
 * @return An error code.
 */
int db_update(struct database *db, const char *key, struct container *c)
{
	return dict_update(db->container, key, c, DICT_PTR);
}

/**
 * @brief Store an entry in a database.
 * @param db Database to store in.
 * @param key Key to store \p data under.
 * @param c Container to store.
 * @return Error code.
 */
int db_store(struct database *db, const char *key, struct container *c)
{
	/*
	 * add the entry to the dictionary.
	 */
	return dict_add(db->container, key, c, DICT_PTR);
}

/**
 * @brief Delete an entry for a given database.
 * @param db Database to delete from.
 * @param key Key to delete.
 * @param data Pointer to store the delete data in.
 * @return Error code.
 *
 * Only trust the data in \p data if the return value is
 * \p -DICT_OK.
 */
int db_delete(struct database *db, const char *key, db_data_t *data)
{
	union entry_data val;
	int rv;

	rv = dict_delete(db->container, key, &val, false);
	memcpy(data, &val, sizeof(val));

	return rv;
}

/**
 * @brief Lookup a database key.
 * @param db Database to perform the lookup on.
 * @param key Key to lookup.
 * @param data Pointer to store data in.
 * @return Error code.
 *
 * Only trust the data in \p data if the return value is \p -DICT_OK.
 */
int db_lookup(struct database *db, const char *key, db_data_t *data)
{
	union entry_data val;
	size_t tmp;
	int rv;

	rv = dict_lookup(db->container, key, &val, &tmp);
	if(rv != -XFIRE_OK)
		return rv;
	else
		memcpy(data, &val, sizeof(val));

	return rv;
}

/**
 * @brief Free an entire database.
 * @param db Database to free.
 */
void db_free(struct database *db)
{
	dict_clear(db->container);
	dict_free(db->container);

	xfire_free(db->name);
	xfire_free(db);
}

/** @} */

