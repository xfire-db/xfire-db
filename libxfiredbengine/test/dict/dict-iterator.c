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


static const char *dbg_keys[] = {"key1","key2","key3","key4","key5","key6","key7",
				"key8","key9","key10","key11","key12",
				};

static char *dbg_values[] = {"val1","val2","val3","val4","val5","val6","val7",
				"val8","val9","val10","val11","val12",
				};

static void dbg_setup_dict(struct dict *d)
{
	int i;

	for(i = 0; i < 12; i++) {
		assert(dict_add(d, dbg_keys[i], dbg_values[i],
				DICT_PTR) == -XFIREDB_OK);
	}
}

static struct dict *strings;

static void setup(struct unit_test *test)
{
	strings = dict_alloc();
	dbg_setup_dict(strings);
}

static void teardown(struct unit_test *test)
{
	dict_clear(strings);
	dict_free(strings);
}

static void test_iterator_forward(void)
{
	struct dict_iterator *it;
	struct dict_entry *e;
	int i = 0;

	it = dict_get_safe_iterator(strings);
	assert(it != NULL);
	e = dict_iterator_next(it);
	assert(e != NULL);

	for(; e; e = dict_iterator_next(it)) {
		printf("Found key: %s\n", e->key);
		i++;
	}

	assert(i == 12);

	dict_iterator_free(it);
}

static void test_iterator_backward(void)
{
	struct dict_iterator *it;
	struct dict_entry *e;
	int i = 0;

	it = dict_get_safe_iterator(strings);
	assert(it != NULL);
	e = dict_iterator_prev(it);
	assert(e != NULL);

	for(; e; e = dict_iterator_prev(it)) {
		printf("Found key: %s\n", e->key);
		i++;
	}

	assert(i == 12);
	dict_iterator_free(it);
}

static test_func_t test_func_array[] = {test_iterator_forward, test_iterator_backward, NULL};
struct unit_test dict_iterator_test = {
	.name = "storage:dict:iterator",
	.setup = setup,
	.teardown = teardown,
	.tests = test_func_array,
};

