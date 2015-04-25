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

static void test_rb_insert(struct rb_root *root, int key)
{
	struct data_node *node;

	node = malloc(sizeof(*node));
	if(!node)
		return;

	rb_init_node(&node->node);
	rb_set_key(&node->node, key);
	node->data = node_data;
	rb_insert(root, &node->node);
}

void *test_thread_a(void *arg)
{
	int idx;

	printf("Thread 1 starting\n");
	for(idx = 11; idx <= 20; idx++)
		test_rb_insert(&root, idx);

	xfire_thread_exit(NULL);
}

void *test_thread_b(void *arg)
{
	int idx;

	printf("Thread 2 starting\n");
	for(idx = 1; idx <= 10; idx++)
		test_rb_insert(&root, idx);

	xfire_thread_exit(NULL);
}

void *test_thread_c(void *arg)
{
	int idx;

	printf("Thread 3 starting\n");
	for(idx = 13; idx <= 20; idx++)
		rb_remove(&root, idx, (char*)node_data);

	xfire_thread_exit(NULL);
}
void *test_thread_d(void *arg)
{
	int idx;

	printf("Thread 4 starting\n");
	for(idx = 3; idx <= 10; idx++)
		rb_remove(&root, idx, (char*)node_data);

	xfire_thread_exit(NULL);
}


int main(int argc, char **argv)
{
	struct thread *a, *b;
	struct rb_node *node;
	struct data_node *dnode;

	memset(&root, 0, sizeof(root));
	root.iterate = &compare_node;
	rb_init_root(&root);

	/* insert */
	a = xfire_create_thread("thread a", &test_thread_b, NULL);
	b = xfire_create_thread("thread b", &test_thread_a, NULL);

	xfire_thread_join(a);
	xfire_thread_join(b);

	xfire_destroy_thread(a);
	xfire_destroy_thread(b);

	rb_dump(&root,stdout);
	
	/* removal */
	a = xfire_create_thread("thread c", &test_thread_c, NULL);
	b = xfire_create_thread("thread d", &test_thread_d, NULL);

	xfire_thread_join(a);
	xfire_thread_join(b);

	xfire_destroy_thread(a);
	xfire_destroy_thread(b);

	node = rb_find(&root, 12);

	if(node) {
		dnode = container_of(node, struct data_node, node);
		printf("Found node: <\"%llu\",\"%s\">\n",
				(unsigned long long)node->key,
				dnode->data);
	} else {
		printf("Node not found!\n");
	}

	rb_dump(&root,stdout);
	fputc('\n', stdout);
	return -EXIT_SUCCESS;
}

