/*
 *  XFireDB string split
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

/**
 * @brief Split a string based on a delimiter.
 * @param a_str String to split.
 * @param a_delim Delimiter to use for splitting \p a_str.
 * @return An array of strings.
 */
char** str_split(char* a_str, const char a_delim)
{
	char** result = 0;
	char *token;
	char* tmp = a_str;
	char* last_comma = 0;
	char delim[2];
	size_t count = 0, idx;

	delim[0] = a_delim;
	delim[1] = 0;
	/* Count how many elements will be extracted. */
	while (*tmp) {
		if (a_delim == *tmp) {
			count++;
			last_comma = tmp;
		}
		tmp++;
	}

	/* Add space for trailing token. */
	count += last_comma < (a_str + strlen(a_str) - 1);

	/* Add space for terminating null string so caller
	knows where the list of returned strings ends. */
	count++;

	result = malloc(sizeof(char*) * count);

	if (result) {
		idx = 0;
		token = strtok(a_str, delim);

		while (token) {
			assert(idx < count);
			*(result + idx++) = strdup(token);
			token = strtok(0, delim);
		}
		assert(idx == count - 1);
		*(result + idx) = 0;
	}

	return result;
}

/**
 * @brief Count how often a given character occurs in a string.
 * @param str String to search in.
 * @param x Character to search for.
 * @return Number of times \p x occurs in \p str.
 */
int str_count_occurences(const char *str, char x)
{
	int rv = 0;

	while(*str++) {
		if(*str == x)
			rv++;
	}

	return rv;
}

