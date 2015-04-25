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

struct rbtree_node {
	struct rbtree node;
	const char *data;
};

static struct rbtree_root root;

static const char node_data[] = "Hello World!";
static const char node_data2[] = "Hello World, again!";

static bool compare_node(struct rbtree *node, const void *arg)
{

	struct rbtree_node *container;

	if(!node)
		return false;

	container = container_of(node, struct rbtree_node, node);
	if(!strcmp(container->data, arg))
		return true;

	return false;
}

static void test_rbtree_insert(struct rbtree_root *root, int key)
{
	struct rbtree_node *node;

	node = malloc(sizeof(*node));
	if(!node)
		return;

	rbtree_init_node(&node->node);
	rbtree_set_key(&node->node, key);
	node->data = node_data;
	rbtree_insert(root, &node->node);
}

void *test_thread_a(void *arg)
{
	int idx;

	printf("Thread 1 starting\n");
	for(idx = 11; idx <= 20; idx++)
		test_rbtree_insert(&root, idx);

	xfire_thread_exit(NULL);
}

void *test_thread_b(void *arg)
{
	int idx;

	printf("Thread 2 starting\n");
	for(idx = 1; idx <= 10; idx++)
		test_rbtree_insert(&root, idx);

	xfire_thread_exit(NULL);
}

void *test_thread_c(void *arg)
{
	int idx;

	printf("Thread 3 starting\n");
	for(idx = 13; idx <= 20; idx++)
		rbtree_remove(&root, idx, (char*)node_data);

	xfire_thread_exit(NULL);
}
void *test_thread_d(void *arg)
{
	int idx;

	printf("Thread 4 starting\n");
	for(idx = 3; idx <= 10; idx++)
		rbtree_remove(&root, idx, (char*)node_data);

	xfire_thread_exit(NULL);
}


int main(int argc, char **argv)
{
	struct thread *a, *b;
	struct rbtree *node;
	struct rbtree_node *dnode;

	memset(&root, 0, sizeof(root));
	root.iterate = &compare_node;
	rbtree_init_root(&root);

	/* insert */
	a = xfire_create_thread("thread a", &test_thread_b, NULL);
	b = xfire_create_thread("thread b", &test_thread_a, NULL);

	xfire_thread_join(a);
	xfire_thread_join(b);

	xfire_destroy_thread(a);
	xfire_destroy_thread(b);

	rbtree_dump(&root,stdout);
	
	/* removal */
	a = xfire_create_thread("thread c", &test_thread_c, NULL);
	b = xfire_create_thread("thread d", &test_thread_d, NULL);

	xfire_thread_join(a);
	xfire_thread_join(b);

	xfire_destroy_thread(a);
	xfire_destroy_thread(b);

	node = rbtree_find(&root, 12);

	if(node) {
		dnode = container_of(node, struct rbtree_node, node);
		printf("Found node: <\"%llu\",\"%s\">\n",
				(unsigned long long)node->key,
				dnode->data);
	} else {
		printf("Node not found!\n");
	}

	rbtree_dump(&root,stdout);
	fputc('\n', stdout);
	return -EXIT_SUCCESS;
}

