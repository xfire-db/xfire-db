/*
 *  String quotation library
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
 * @addtogroup stdlib
 * @{
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <xfiredb/xfiredb.h>
#include <xfiredb/error.h>
#include <xfiredb/mem.h>

/**
 * @brief Free an allocated string.
 * @param str String to free.
 *
 * Free a string allocated by xfiredb_escape_string or xfiredb_unescape_string.
 */
void xfiredb_escape_free(char *str)
{
	xfiredb_free(str);
}

static void xfiredb_unescape_char(char *c, char *dst)
{
	switch(*c) {
	case 'a':
		*dst = '\a';
		break;
	case 'b':
		*dst = '\b';
		break;
	case 'f':
		*dst = '\f';
		break;
	case 'n':
		*dst = '\n';
		break;
	case 'r':
		*dst = '\r';
		break;
	case 't':
		*dst = '\t';
		break;
	case 'v':
		*dst = '\v';
		break;
	default:
		*dst = *c;
		break;
	}
}

/**
 * @brief Unescape a string.
 * @param src String to unescape.
 * @note Only feed strings escaped by xfiredb_escape_string to this function.
 * @note The returned string is allocated using malloc.
 * @see xfiredb_escape_free
 */
char *xfiredb_unescape_string(char *src)
{
	char *dst, *orig, c;
	int len;

	len = strlen(src);
	orig = dst = xfiredb_zalloc(len);

	while((c = *(src++)) != '\0') {
		switch(c) {
		case '\\':
			xfiredb_unescape_char(src, dst);
			dst++;
			src++;
			break;
		default:
			*(dst++) = c;
			break;
		}
	}

	*dst = '\0';
	len = strlen(orig);
	return xfiredb_realloc(orig, len);

}

/**
 * @brief Escape a string.
 * @param src String to escape.
 * @return The escaped version of \p src.
 * @see xfiredb_unescape_string xfiredb_escape_free
 * @note The return value is allocated by malloc.
 */
char *xfiredb_escape_string(char *src)
{
	int len;
	char *orig, *dst, c;

	len = strlen(src);
	orig = dst = xfiredb_zalloc(len * 2);

	while((c = *(src++)) != '\0') {
		switch(c) {
		case '\a':
			*(dst++) = '\\';
			*(dst++) = 'a';
			break;
		case '\b':
			*(dst++) = '\\';
			*(dst++) = 'b';
			break;
		case '\f':
			*(dst++) = '\\';
			*(dst++) = 'f';
			break;
		case '\n':
			*(dst++) = '\\';
			*(dst++) = 'n';
			break;
		case '\r':
			*(dst++) = '\\';
			*(dst++) = 'r';
			break;
		case '\t':
			*(dst++) = '\\';
			*(dst++) = 't';
			break;
		case '\v':
			*(dst++) = '\\';
			*(dst++) = 'v';
			break;
		case '\\':
			*(dst++) = '\\';
			*(dst++) = '\\';
			break;
		case '\"':
			*(dst++) = '\\';
			*(dst++) = '\"';
			break;
		case '\'':
			*(dst++) = '\\';
			*(dst++) = '\'';
			break;
		default:
			*(dst++) = c;
			break;
		}
	}

	*dst = '\0';

	len = strlen(orig);
	return xfiredb_realloc(orig, len);
}

/** @} */

