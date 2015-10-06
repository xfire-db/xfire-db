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
#include <xfire/xfire.h>
#include <xfire/types.h>
#include <xfire/dict.h>

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

CDECL
extern struct database *db_alloc(const char *name);
extern void db_free(struct database *db);

extern int db_store(struct database *db, const char *key, void *data);
extern int db_delete(struct database *db, const char *key, db_data_t *data);
CDECL_END

#endif

/** @} */

