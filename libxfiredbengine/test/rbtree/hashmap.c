/*
 *  RBTree test
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
#include <xfiredb/hashmap.h>
#include <xfiredb/mem.h>

static struct string s1, s2, s3, s4;

static void test_hm_insert(struct hashmap *map)
{
	string_init(&s1);
	string_init(&s2);
	string_init(&s3);
	string_init(&s4);

	string_set(&s1, "test-val-1");
	string_set(&s2, "test-val-2");
	string_set(&s3, "test-val-3");
	string_set(&s4, "test-val-4");

	assert(hashmap_add(map, "key1", &s1.node) == -XFIRE_OK);
	assert(hashmap_add(map, "key2", &s2.node) == -XFIRE_OK);
	assert(hashmap_add(map, "key3", &s3.node) == -XFIRE_OK);
	assert(hashmap_add(map, "key4", &s4.node) == -XFIRE_OK);
}

static int iterate_count, free_count;

static struct hashmap map;

void setup(void)
{
	hashmap_init(&map);
	test_hm_insert(&map);
	iterate_count = 0;
	free_count = 0;
}

void teardown(void)
{
	struct hashmap_node *hnode;
	struct string *s;

	for(hnode = hashmap_clear_next(&map);
			hnode; hnode = hashmap_clear_next(&map)) {
		s = container_of(hnode, struct string, node);
		hashmap_node_destroy(hnode);
		string_destroy(s);
		free_count++;
	}
	hashmap_destroy(&map);
	assert(free_count == 4);
}

void test_hashmap(void)
{
	struct hashmap_node *node;
	struct string *s;
	struct hashmap_iterator *it;

	node = hashmap_find(&map, "key4");
	s = container_of(node, struct string, node);
	assert(!strcmp(s->str, "test-val-4"));
	it = hashmap_new_iterator(&map);
	for(node = hashmap_iterator_next(it); node;
			node = hashmap_iterator_next(it)) {
		iterate_count++;
	}
	hashmap_free_iterator(it);
	assert(iterate_count == 4);
}

test_func_t test_func_array[] = {test_hashmap, NULL};
const char *test_name = "Hashmap test";

