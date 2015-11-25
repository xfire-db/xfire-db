/*
 *  Set test
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
#include <unittest.h>

#include <xfiredb/xfiredb.h>
#include <xfiredb/error.h>
#include <xfiredb/string.h>
#include <xfiredb/set.h>
#include <xfiredb/mem.h>

static void test_set_insert(struct set *set)
{
	struct set_key *k1, *k2, *k3, *k4;

	k1 = xfiredb_zalloc(sizeof(*k1));
	k2 = xfiredb_zalloc(sizeof(*k2));
	k3 = xfiredb_zalloc(sizeof(*k3));
	k4 = xfiredb_zalloc(sizeof(*k4));

	assert(set_add(set, "key1", k1) == -XFIREDB_OK);
	assert(set_add(set, "key2", k2) == -XFIREDB_OK);
	assert(set_add(set, "key3", k3) == -XFIREDB_OK);
	assert(set_add(set, "key4", k4) == -XFIREDB_OK);
}

static int iterate_count, free_count;
static struct set set;

static void setup(struct unit_test *t)
{
	set_init(&set);
	test_set_insert(&set);
	iterate_count = 0;
	free_count = 0;
	xfiredb_set_loadstate(true);
}

static void teardown(struct unit_test *t)
{
	struct set_key *k;
	struct set_iterator *it;

	it = set_iterator_new(&set);
	for_each_set(&set, k, it) {
		iterate_count++;
	}

	set_iterator_free(it);
	assert(iterate_count == 4);
	set_clear(&set);
}

void test_set(void)
{
	assert(set_contains(&set, "key1"));
	assert(set_contains(&set, "key2"));
	assert(set_contains(&set, "key3"));
	assert(set_contains(&set, "key4"));
}

static test_func_t test_func_array[] = {test_set, NULL};
struct unit_test rb_set_test = {
	.name = "storage:red-black:set",
	.setup = setup,
	.teardown = teardown,
	.tests = test_func_array,
};
