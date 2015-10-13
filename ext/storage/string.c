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

#include <xfire/xfire.h>
#include <xfire/types.h>
#include <xfire/os.h>
#include <xfire/string.h>
#include <xfire/list.h>
#include <xfire/mem.h>

/**
 * @brief Initialise a new string container.
 * @param str String to initialise.
 */
void string_init(struct string *str)
{
	str->str = NULL;
	str->len = 0UL;
	xfire_spinlock_init(&str->lock);
	list_node_init(&str->entry);
	rb_init_node(&str->node);
}

/**
 * @brief Allocate a string container.
 * @param len Length of the string.
 * @return The allocated string container.
 */
struct string *string_alloc(size_t len)
{
	struct string *string;

	string = xfire_zalloc(sizeof(*string));
	string_init(string);

	if(len)
		string->str = xfire_zalloc(len);

	string->len = len;

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
	xfire_spin_lock(&string->lock);
	string->str = xfire_realloc(string->str, len);

	memcpy(string->str, str, len);
	string->len = len;
	xfire_spin_unlock(&string->lock);
}

/**
 * @brief Get the c string contained in \p string.
 * @param str String to copy in.
 * @param buff Buffer to copy into.
 * @param num Length of buff in bytes.
 * @return Error code. 0 on success, -1 otherwise.
 */
int string_get(struct string *str, char **buff)
{
	xfire_spin_lock(&str->lock);
	xfire_sprintf(buff, "%s", str->str);
	xfire_spin_unlock(&str->lock);

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

	xfire_spin_lock(&str->lock);
	len = str->len;
	xfire_spin_unlock(&str->lock);

	return len;
}

/**
 * @brief Destroy a string container.
 * @param str String to destroy.
 */
void string_destroy(struct string *str)
{
	if(str->str)
		xfire_free(str->str);

	rb_node_destroy(&str->node);
	xfire_spinlock_destroy(&str->lock);
}

/**
 * @brief Free the memory in use by the given string container.
 * @param string String to free.
 */
void string_free(struct string *string)
{
	string_destroy(string);
	xfire_free(string);
}

/** @} */

