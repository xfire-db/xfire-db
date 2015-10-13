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
#include <math.h>

#include <xfire/hashmap.h>
#include <xfire/mem.h>

static void test_hm_insert(struct hashmap *map)
{
	hashmap_add(map, "key1", "test-val-1");
	hashmap_add(map, "key2", "test-val-2");
	hashmap_add(map, "key3", "test-val-3");
	hashmap_add(map, "key4", "test-val-4");
}

static void iterate_hook(struct hashmap *map, struct hashmap_node *node)
{
	printf("Iterating key: %s\n", node->key);
}

int main(int argc, char **argv)
{
	struct hashmap map;
	char *test;

	hashmap_init(&map);
	test_hm_insert(&map);
	hashmap_find(&map, "key4", &test);
	printf("Found value: %s\n", test);
	xfire_free(test);

	hashmap_iterate(&map, &iterate_hook);

	hashmap_destroy(&map);
	return -EXIT_SUCCESS;
}

