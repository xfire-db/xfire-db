/*
 *  Concurrent RBTree test
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

#include <xfiredb/rbtree.h>
#include <xfiredb/mem.h>
#include <xfiredb/os.h>

struct data_node {
	struct rb_node node;
	const char *data;
};

static const char node_data1[] = "Hello World!";
static const char node_data2[] = "Hello World 2!";
static const char node_data3[] = "Hello World 3!";

static struct rb_root root;

static bool compare_node(struct rb_node *node, const void *arg)
{

	struct data_node *container;

	container = container_of(node, struct data_node, node);
	if(!strcmp(container->data, arg))
		return true;

	return false;
}

static void test_rb_insert(struct rb_root *root, int key)
{
	struct data_node *node;
	struct rb_node *tmp;

	node = xfire_zalloc(sizeof(*node));
	if(!node)
		return;

	rb_init_node(&node->node);
	rb_set_key(&node->node, key);
	node->data = node_data1;
	tmp = rb_insert(root, &node->node, false);
	node = container_of(tmp, struct data_node, node);
	assert(!strcmp(node->data, node_data1));
}

static void test_insert_duplicate(struct rb_root *root, int key,
		const char *data)
{
	struct data_node *node;
	struct rb_node *tmp;

	node = xfire_zalloc(sizeof(*node));
	if(!node)
		return;

	rb_init_node(&node->node);
	rb_set_key(&node->node, key);
	node->data = data;
	tmp = rb_insert(root, &node->node, true);
	node = container_of(tmp, struct data_node, node);
	assert(!strcmp(node->data, data));
}

static void *test_thread_b(void *arg)
{
	int idx;

	for(idx = 31; idx <= 40; idx++)
		test_rb_insert(&root, idx);

	xfire_thread_exit(NULL);
}

static void *test_thread_a(void *arg)
{
	int idx;

	for(idx = 21; idx <= 30; idx++)
		test_rb_insert(&root, idx);

	test_insert_duplicate(&root, 24, node_data3);
	xfire_thread_exit(NULL);
}

static void *test_thread_c(void *arg)
{
	int idx;

	rb_remove(&root, 18, (char*)node_data1);
	for(idx = 11; idx <= 20; idx++)
		rb_remove(&root, idx, (char*)node_data1);

	xfire_thread_exit(NULL);
}

static void *test_thread_d(void *arg)
{
	int idx;

	rb_remove(&root, 18, (char*)node_data2);
	for(idx = 1; idx <= 10; idx++)
		rb_remove(&root, idx, (char*)node_data1);

	xfire_thread_exit(NULL);
}

static void rb_setup_tree(void)
{
	int idx;

	for(idx = 1; idx <= 20; idx++)
		test_rb_insert(&root, idx);

	test_insert_duplicate(&root, 18, node_data2);
	test_insert_duplicate(&root, 18, node_data3);
}

static void test_find_node(u64 key, const void *data)
{
	struct data_node *dnode1;
	struct rb_node *node1;

	node1 = rb_find_duplicate(&root, key, data);
	if(node1) {
		dnode1 = container_of(node1, struct data_node, node);
		assert(!strcmp(dnode1->data, data));
	} else {
		exit(-EXIT_FAILURE);
	}
}

static void setup(struct unit_test *t)
{
	memset(&root, 0, sizeof(root));
	rb_init_root(&root);
	root.cmp = &compare_node;
	rb_setup_tree();
}

static void test_rb_concurrent(void)
{
	struct thread *a, *b, *c, *d;

	a = xfire_create_thread("thread a", &test_thread_a, NULL);
	b = xfire_create_thread("thread b", &test_thread_b, NULL);
	c = xfire_create_thread("thread c", &test_thread_c, NULL);
	d = xfire_create_thread("thread d", &test_thread_d, NULL);

	xfire_thread_join(a);
	xfire_thread_join(b);
	xfire_thread_join(c);
	xfire_thread_join(d);

	xfire_destroy_thread(a);
	xfire_destroy_thread(b);
	xfire_destroy_thread(c);
	xfire_destroy_thread(d);

	test_find_node(24, node_data3);
	test_find_node(18, node_data3);
}

static void teardown(struct unit_test *t)
{
	rb_destroy_root(&root);
}

static test_func_t test_func_array[] = {test_rb_concurrent, NULL};
struct unit_test rb_concurrent_test = {
	.name = "storage:red-black:concurrent",
	.setup = setup,
	.teardown = teardown,
	.tests = test_func_array,
};

