/*
 *  String storage
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

#ifndef __STRING_H__
#define __STRING_H__

/**
 * @addtogroup string
 * @{
 */

#include <xfiredb/engine/xfiredb.h>
#include <xfiredb/engine/types.h>
#include <xfiredb/engine/os.h>
#include <xfiredb/engine/list.h>
#include <xfiredb/engine/rbtree.h>
#include <xfiredb/engine/hashmap.h>

/**
 * @brief String container.
 */
struct string {
	struct list entry; //!< List entry.
	struct hashmap_node node; //!< Map node.

	char *str; //!< String pointer.
	size_t len; //!< Length of \p str in bytes.
	xfire_spinlock_t lock; //!< Lock.
};

/**
 * @brief String magic. Used to differentiate strings and lists.
 */
#define S_MAGIC 0x785A06B8

CDECL
extern struct string *string_alloc(const char *data);
extern void string_init(struct string *str);
extern void string_free(struct string *string);
extern void string_destroy(struct string *str);
extern void string_set(struct string *string, const char *str);
extern int string_get(struct string *str, char **buf);
extern size_t string_length(struct string *str);

/**
 * @brief Get the c string from a string container.
 * @param str String container.
 * @return The c string contained in \p str.
 */
static inline void *string_data(struct string *str)
{
	return str->str;
}

CDECL_END

/** @} */
#endif