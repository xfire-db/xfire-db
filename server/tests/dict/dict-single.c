/*
 *  Hashed dictionary unit test
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

#include <sys/time.h>

#include <xfire/xfire.h>
#include <xfire/types.h>
#include <xfire/dict.h>

#define KEY_1 "test::key::1"
#define TEST_1 "Test string one"

#define KEY_2 "test::key::2"
#define TEST_2 "Test string two"

#define KEY_3 "test::key::3"
#define TEST_3 "Test string three"

#define KEY_4 "test::key::4"
#define TEST_4 "Test string four"

#define KEY_5 "test::key::5"
#define TEST_5 "Test string five"

static void dot()
{
	fputs(".\n", stdout);
}

static void dbg_setup_dict(struct dict *d)
{
	dict_add(d, KEY_1, TEST_1, DICT_PTR);
	dict_add(d, KEY_2, TEST_2, DICT_PTR);
	dict_add(d, KEY_3, TEST_3, DICT_PTR);
	dict_add(d, KEY_4, TEST_4, DICT_PTR);
	dict_add(d, KEY_5, TEST_5, DICT_PTR);
}

static void dbg_empty_dict(struct dict *d)
{
	dict_delete(d, KEY_1, false);
	dict_delete(d, KEY_2, false);
	dict_delete(d, KEY_3, false);
	dict_delete(d, KEY_4, false);
	dict_delete(d, KEY_5, false);
}

int main(int argc, char **argv)
{
	struct dict *strings;

	printf("Creating dictionary\n");
	strings = dict_alloc();

	if(!strings) {
		printf("Creating dictionary failed.\nDictionary test failed\n");
		return -EXIT_FAILURE;
	}

	printf("Storing and retrieving data\n.\n.\n.\n.\n");

	dbg_setup_dict(strings);
	dbg_empty_dict(strings);

	printf("Destroying dictionary\n");
	dict_free(strings);

	return -EXIT_SUCCESS;
}

