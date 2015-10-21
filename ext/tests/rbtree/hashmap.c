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

#include <xfire/string.h>
#include <xfire/hashmap.h>
#include <xfire/mem.h>

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

	hashmap_add(map, "key1", &s1.node);
	hashmap_add(map, "key2", &s2.node);
	hashmap_add(map, "key3", &s3.node);
	hashmap_add(map, "key4", &s4.node);
}

static void iterate_hook(struct hashmap *map, struct hashmap_node *node)
{
	printf("Iterating key: %s\n", node->key);
}

static void hm_free_hook(struct hashmap_node *n)
{
	struct string *s;

	s = container_of(n, struct string, node);
	string_destroy(s);
}

int main(int argc, char **argv)
{
	struct hashmap map;
	struct hashmap_node *node;
	struct string *s;

	hashmap_init(&map);
	test_hm_insert(&map);
	node = hashmap_find(&map, "key4");
	s = container_of(node, struct string, node);
	printf("Found value: %s\n", s->str);

	hashmap_iterate(&map, &iterate_hook);
	hashmap_clear(&map, &hm_free_hook);
	hashmap_destroy(&map);
	return -EXIT_SUCCESS;
}

