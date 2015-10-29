/*
 *  Database header
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

#ifndef __DATABASE_H__
#define __DATABASE_H__

#include <xfiredb/xfiredb.h>
#include <xfiredb/types.h>
#include <xfiredb/dict.h>
#include <xfiredb/container.h>

/**
 * @brief Database data type.
 */
typedef union entry_data db_data_t;

/**
 * @brief Database type.
 */
struct database {
	char *name; //!< Database name.
	struct dict *container; //!< Data container.
};

#define db_iterator dict_iterator //!< Iterator typedef
#define db_entry dict_entry //!< Data entry typedef

/**
 * @brief Get a database iterator.
 * @param __db Database to get an iterator for.
 */
#define db_get_iterator(__db) \
	dict_get_safe_iterator((__db)->container)
/**
 * @brief Get the next entry from an iterator.
 * @param __it Iterator to get the next from.
 */
#define db_iterator_next(__it) \
	dict_iterator_next(__it)
/**
 * @brief Release an iterator.
 * @param __it Iterator to release.
 */
#define db_iterator_free(__it) \
	dict_iterator_free(__it)

CDECL
/**
 * @brief Get the number of elements stored in \p db.
 * @param db Database to get the size of.
 * @return The number of elements in \p db.
 */
static inline long db_get_size(struct database *db)
{
	return dict_get_size(db->container);
}

extern struct database *db_alloc(const char *name);
extern void db_free(struct database *db);
extern int db_update(struct database *db,
		const char *key, struct container *c);

extern int db_store(struct database *db, const char *key, struct container *c);
extern int db_delete(struct database *db, const char *key, db_data_t *data);
extern int db_lookup(struct database *db, const char *key, db_data_t *data);
CDECL_END

#endif

/** @} */

