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
#include <math.h>

#include <xfire/rbtree.h>
#include <xfire/os.h>

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

	node = malloc(sizeof(*node));
	if(!node)
		return;

	rb_init_node(&node->node);
	rb_set_key(&node->node, key);
	node->data = node_data1;
	rb_insert(root, &node->node, false);
}

static void test_insert_duplicate(struct rb_root *root, int key,
		const char *data)
{
	struct data_node *node;

	node = malloc(sizeof(*node));
	if(!node)
		return;

	rb_init_node(&node->node);
	rb_set_key(&node->node, key);
	node->data = data;
	rb_insert(root, &node->node, true);
}

void *test_thread_b(void *arg)
{
	int idx;

	printf("Thread 2 starting\n");
	for(idx = 31; idx <= 40; idx++)
		test_rb_insert(&root, idx);

	xfire_thread_exit(NULL);
}

void *test_thread_a(void *arg)
{
	int idx;

	printf("Thread 1 starting\n");
	for(idx = 21; idx <= 30; idx++)
		test_rb_insert(&root, idx);

	test_insert_duplicate(&root, 24, node_data3);
	xfire_thread_exit(NULL);
}

void *test_thread_c(void *arg)
{
	int idx;

	printf("Thread 3 starting\n");
	rb_remove(&root, 18, (char*)node_data1);
	rb_remove(&root, 18, (char*)node_data3);
	for(idx = 11; idx <= 20; idx++)
		rb_remove(&root, idx, (char*)node_data1);

	xfire_thread_exit(NULL);
}

void *test_thread_d(void *arg)
{
	int idx;

	printf("Thread 4 starting\n");
	rb_remove(&root, 18, (char*)node_data2);
	for(idx = 1; idx <= 10; idx++)
		rb_remove(&root, idx, (char*)node_data1);

	xfire_thread_exit(NULL);
}

void rb_setup_tree(void)
{
	int idx;

	for(idx = 1; idx <= 20; idx += 100)
		test_rb_insert(&root, idx);

	test_insert_duplicate(&root, 18, node_data2);
	test_insert_duplicate(&root, 18, node_data3);
}

int main(int argc, char **argv)
{
	struct thread *a, *b, *c, *d;
	struct rb_node *node1, *node2;
	struct data_node *dnode1, *dnode2;
	s64 num;
	s32 height;

	memset(&root, 0, sizeof(root));
	rb_init_root(&root);
	root.iterate = &compare_node;

	rb_setup_tree();

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

	node1 = rb_find_duplicate(&root, 24, &compare_node,
			(char*)node_data3);

	node2 = rb_find_duplicate(&root, 23, &compare_node,
			(char*)node_data1);

	if(node1) {
		dnode1 = container_of(node1, struct data_node, node);
		printf("Found node: <\"%llu\",\"%s\">\n",
				(unsigned long long)node1->key,
				dnode1->data);
	} else {
		printf("Node not found!\n");
	}

	if(node2) {
		dnode2 = container_of(node2, struct data_node, node);
		printf("Found node: <\"%llu\",\"%s\">\n",
				(unsigned long long)node2->key,
				dnode2->data);
	} else {
		printf("Node not found!\n");
	}

	num = rb_get_size(&root);
	height = rb_get_height(&root);
	printf("Number of nodes: %lld - Tree height: %d\n",
			(long long)num, (int)height);
	rb_dump(&root,stdout);
	return -EXIT_SUCCESS;
}

