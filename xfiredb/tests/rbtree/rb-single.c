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
#include <unittest.h>

#include <xfire/rbtree.h>
#include <xfire/os.h>

struct data_node {
	struct rb_node node;
	const char *data;
};

static struct rb_root root;

static const char node_data[] = "Hello World!";
static const char node_data2[] = "Hello World, again!";

static bool compare_node(struct rb_node *node, const void *arg)
{

	struct data_node *container;

	if(!node)
		return false;

	container = container_of(node, struct data_node, node);
	if(!strcmp(container->data, arg))
		return true;

	return false;
}

static void dbg_rb_insert(struct rb_root *root, int key)
{
	struct data_node *node;
	struct rb_node *tmp;

	node = malloc(sizeof(*node));
	if(!node)
		return;

	rb_init_node(&node->node);
	rb_set_key(&node->node, key);

	if(key == 27)
		node->data = node_data2;
	else
		node->data = node_data;

	tmp = rb_insert(root, &node->node, false);
	assert(tmp != NULL);
	assert(tmp->key == key);
}

static void dbg_iterate_tree(struct rb_root *root)
{
	struct rb_iterator *it = rb_new_iterator(root);
	struct rb_node *carriage;

	printf("Length = %lli\n", (long long)rb_get_size(root));
	for(carriage = rb_iterator_next(it);
			carriage; carriage = rb_iterator_next(it)) {
		rb_remove(root, carriage->key, (void*)node_data);
	}
	rb_free_iterator(it);
	printf("Length = %lli\n", (long long)rb_get_size(root));
}

static void dbg_setup_tree(void)
{
	int idx;

	for(idx = 1; idx <= 20; idx++)
		dbg_rb_insert(&root, idx);
}

static void dbg_remove_tree(void)
{
	int idx;

	for(idx = 11; idx <= 20; idx++)
		assert(rb_remove(&root, idx, (void*)node_data) != NULL);
}

static void dbg_tree_incremental(void)
{
	int i;

	for(i = 1; i <= 10; i++)
		dbg_rb_insert(&root, i);
}

static void dbg_tree_random(void)
{
	struct rb_node *find;
	struct data_node *data;

	dbg_rb_insert(&root, 20);;
	dbg_rb_insert(&root, 10);
	dbg_rb_insert(&root, 30);
	dbg_rb_insert(&root, 40);
	dbg_rb_insert(&root, 27);
	dbg_rb_insert(&root, 25);
	dbg_rb_insert(&root, 28);
	dbg_rb_insert(&root, 26);

	find = rb_find_duplicate(&root, 27, node_data2);
	data = container_of(find, struct data_node, node);
	assert(!strcmp(data->data, node_data2));
}

void setup(void)
{
	memset(&root, 0, sizeof(root));
	root.cmp = &compare_node;
	rb_init_root(&root);
}

void test_rb_remove(void)
{
	dbg_setup_tree();
	dbg_remove_tree();
}

void test_rb_insert(void)
{
	dbg_tree_incremental();
	dbg_tree_random();
}

void test_rb_iterate(void)
{
	dbg_setup_tree();
	dbg_iterate_tree(&root);
}

void teardown(void)
{
	rb_destroy_root(&root);
}

test_func_t test_func_array[] = {test_rb_iterate, 
	test_rb_insert, test_rb_remove, NULL};
const char *test_name = "Red-black test";

