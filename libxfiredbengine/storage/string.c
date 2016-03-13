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

/**
 * @addtogroup string
 * @{
 */

#include <xfiredb/xfiredb.h>
#include <xfiredb/types.h>
#include <xfiredb/object.h>
#include <xfiredb/os.h>
#include <xfiredb/string.h>
#include <xfiredb/list.h>
#include <xfiredb/mem.h>

/**
 * @brief Initialise a new string container.
 * @param str String to initialise.
 */
void string_init(struct string *str)
{
	str->str = NULL;
	str->len = 0UL;
	xfiredb_spinlock_init(&str->lock);
	list_node_init(&str->entry);
}

/**
 * @brief Allocate a string container.
 * @param data String data to allocate a string object 'around'.
 * @return The allocated string container.
 */
struct string *string_alloc(const char *data)
{
	struct string *string;
	int len;

	len = strlen(data);
	string = xfiredb_zalloc(sizeof(*string));
	string_init(string);

	/* allocate the length of data + 1 (for the terminator) */
	string->str = xfiredb_zalloc(len + 1);
	string->len = len;
	memcpy(string->str, data, len);

	return string;
}

/**
 * @brief Copy the data of a c string into a string container.
 * @param string String container to copy into.
 * @param str C string which has to be copied into \p string.
 */
void string_set(struct string *string, const char *str)
{
	int len;

	len = strlen(str) + 1;
	xfiredb_spin_lock(&string->lock);
	string->str = xfiredb_realloc(string->str, len);

	memcpy(string->str, str, len);
	string->len = len;
	xfiredb_spin_unlock(&string->lock);
}

/**
 * @brief Get the c string contained in \p string.
 * @param str String to copy in.
 * @param buff Pointer pointer to a buffer to store the string in.
 * @return Error code. 0 on success, -1 otherwise.
 * @note The string is copied into \p buffer. The caller is responsible for
 * freeing up the memory again using xfiredb_free.
 */
int string_get(struct string *str, char **buff)
{
	xfiredb_spin_lock(&str->lock);
	xfiredb_sprintf(buff, "%s", str->str);
	xfiredb_spin_unlock(&str->lock);

	return 0;
}

/**
 * @brief Get the length of a string.
 * @param str String to get the length.
 * @return Length of \p string.
 */
size_t string_length(struct string *str)
{
	size_t len;

	xfiredb_spin_lock(&str->lock);
	len = str->len;
	xfiredb_spin_unlock(&str->lock);

	return len;
}

/**
 * @brief Destroy a string container.
 * @param str String to destroy.
 */
void string_destroy(struct string *str)
{
	if(str->str)
		xfiredb_free(str->str);

	xfiredb_spinlock_destroy(&str->lock);
}

/**
 * @brief Free the memory in use by the given string container.
 * @param string String to free.
 */
void string_free(struct string *string)
{
	string_destroy(string);
	xfiredb_free(string);
}

/** @} */

