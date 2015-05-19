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

#include <xfire/xfire.h>
#include <xfire/types.h>
#include <xfire/os.h>
#include <xfire/string.h>
#include <xfire/mem.h>

void string_init(struct string *str)
{
	str->str = NULL;
	str->len = 0UL;
	xfire_spinlock_init(&str->lock);
}

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

int string_get(struct string *str, char *buff, size_t num)
{
	xfire_spin_lock(&str->lock);
	if(num < str->len) {
		xfire_spin_unlock(&str->lock);
		return -1;
	}

	memcpy(buff, str->str, num);
	xfire_spin_unlock(&str->lock);

	return 0;
}

size_t string_length(struct string *str)
{
	size_t len;

	xfire_spin_lock(&str->lock);
	len = str->len;
	xfire_spin_unlock(&str->lock);

	return len;
}

void string_destroy(struct string *str)
{
	if(str->str)
		xfire_free(str->str);

	xfire_spinlock_destroy(&str->lock);
}

void string_free(struct string *string)
{
	string_destroy(string);
	xfire_free(string);
}

