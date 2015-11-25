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
#include <unittest.h>

#include <sys/time.h>

#include <xfiredb/xfiredb.h>
#include <xfiredb/types.h>
#include <xfiredb/dict.h>

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

#define UPDATE_DATA "Test string 3.1"

static void dbg_setup_dict(struct dict *d)
{
	union entry_data val;
	size_t size;

	assert(dict_add(d, KEY_1, TEST_1, DICT_PTR) == -XFIREDB_OK);
	assert(dict_add(d, KEY_2, TEST_2, DICT_PTR) == -XFIREDB_OK);
	assert(dict_add(d, KEY_3, TEST_3, DICT_PTR) == -XFIREDB_OK);
	assert(dict_add(d, KEY_4, TEST_4, DICT_PTR) == -XFIREDB_OK);
	assert(dict_add(d, KEY_5, TEST_5, DICT_PTR) == -XFIREDB_OK);

	dict_lookup(d, KEY_3, &val, &size);
	assert(!strcmp(TEST_3, val.ptr));
	assert(dict_update(d, KEY_3, UPDATE_DATA, DICT_PTR) == -XFIREDB_OK);

	dict_lookup(d, KEY_3, &val, &size);
	assert(!strcmp(UPDATE_DATA, val.ptr));
}

static void dbg_empty_dict(struct dict *d)
{
	union entry_data val;

	assert(dict_delete(d, KEY_1, &val, false) == -XFIREDB_OK);
	assert(dict_delete(d, KEY_2, &val, false) == -XFIREDB_OK);
	assert(dict_delete(d, KEY_3, &val, false) == -XFIREDB_OK);
	assert(dict_delete(d, KEY_4, &val, false) == -XFIREDB_OK);
	assert(dict_delete(d, KEY_5, &val, false) == -XFIREDB_OK);
}

static struct dict *strings;
static void setup(struct unit_test *test)
{
	strings = dict_alloc();
}

static void test_dict(void)
{
	dbg_setup_dict(strings);
	dbg_empty_dict(strings);
}

static void teardown(struct unit_test *test)
{
	dict_free(strings);
}

static test_func_t test_func_array[] = {test_dict, NULL};
struct unit_test dict_single_test = {
	.name = "storage:dict:single",
	.setup = setup,
	.teardown = teardown,
	.tests = test_func_array,
};

