/*
 *  Skiplist single threaded unit test
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

#include <xfiredb/xfiredb.h>
#include <xfiredb/types.h>
#include <xfiredb/skiplist.h>

struct test_node {
	struct skiplist_node node;
	int i;
};

struct test_node a = {
	.i = 1,
};

struct test_node b = {
	.i = 2,
};

struct test_node c = {
	.i = 3,
};

struct test_node d = {
	.i = 4,
};

static struct skiplist list;
static void setup(struct unit_test *test)
{
	skiplist_init(&list);
}

static void test_skiplist(void)
{
	struct skiplist_node *node;
	struct skiplist_iterator *it;
	s32 size = 4;

	skiplist_insert(&list, "costarring", &a.node);
	skiplist_insert(&list, "liquid", &b.node);
	skiplist_insert(&list, "key3", &c.node);
	skiplist_insert(&list, "hulk", &d.node);

	it = skiplist_iterator_new(&list);
	while((node = skiplist_iterator_next(it)) != NULL) {
		skiplist_iterator_delete(it);
		assert(skiplist_size(&list) == --size);
	}

	skiplist_iterator_free(it);
	skiplist_destroy(&list);
}

static void teardown(struct unit_test *test)
{
}

static test_func_t test_func_array[] = {test_skiplist, NULL};
struct unit_test skiplist_single_test = {
	.name = "storage:skiplist:single",
	.setup = setup,
	.teardown = teardown,
	.tests = test_func_array,
};

