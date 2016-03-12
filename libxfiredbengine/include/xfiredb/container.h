/*
 *  Containers
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
 * @addtogroup container
 * @{
 */

#ifndef __CONTAINER_H__
#define __CONTAINER_H__

#include <xfiredb/xfiredb.h>
#include <xfiredb/error.h>
#include <xfiredb/types.h>
#include <xfiredb/list.h>
#include <xfiredb/string.h>
#include <xfiredb/hashmap.h>
#include <xfiredb/set.h>

#define CONTAINER_STRING_MAGIC 0xFFAABBCC
#define CONTAINER_LIST_MAGIC   0xEEAABBCC
#define CONTAINER_HM_MAGIC     0xDDAABBCC

/**
 * @brief Container type type definition.
 */
typedef enum {
	CONTAINER_STRING, //!< String container.
	CONTAINER_LIST, //!< List container.
	CONTAINER_HASHMAP, //!< Hashmap container.
	CONTAINER_SET, //!< Set container.
} container_type_t;

/**
 * @brief Container data structure.
 */
struct container {
	container_type_t type; //!< Container type.

	union {
		struct list_head list; //!< List head.
		struct string string; //!< String.
		struct hashmap map; //!< Hashmap.
		struct set set; //!< Set.
	} data; //!< Container data union.
};

CDECL
/**
 * @brief Check the type of a container.
 * @param c Container to check.
 * @param type Type to check \p c against.
 * @return TRUE if \p type and the container are equal, FALSE otherwise.
 */
static inline bool container_check_type(struct container *c, container_type_t type)
{
	return c->type == type ? true : false;
}

extern void container_init(struct container *c, container_type_t type);
extern void *container_get_data(struct container *c);
extern void container_destroy(struct container *c);
extern struct container *container_alloc(container_type_t type);
extern struct object *container_to_object(struct container *c);
CDECL_END

#endif

/** @} */

