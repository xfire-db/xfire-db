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

#include <xfire/rbtree.h>

struct rbtree_node {
	struct rbtree node;
	const char *data;
};

static bool duplicate_compare(struct rbtree *node, void *arg)
{
	struct rbtree_node *container;

	container = container_of(node, struct rbtree_node, node);
	if(!strcmp(container->data, arg))
		return true;

	return false;
}

static void test_tree_increasing(void)
{
	struct rbtree_root root;
	struct rbtree_node *node;
	struct rbtree *find;
	int i = 1;

	memset(&root, 0, sizeof(root));

	for(; i <= 9; i++) {
		node = malloc(sizeof(*node));
		rbtree_set_key(&node->node, i);
		node->data = "Hello World!";
		rbtree_insert_duplicate(&root, &node->node);

		if(i == 5) {
			node = malloc(sizeof(*node));
			rbtree_set_key(&node->node, i);
			node->data = "Hello World 2!";
			rbtree_insert_duplicate(&root, &node->node);
		}
	}

	find = rbtree_find_duplicate(&root, 5, &duplicate_compare,
			"Hello World 2!");
	if(find) {
		node = container_of(find, struct rbtree_node, node);
		printf("Found node %llu::%s\n", (unsigned long long)find->key,
				node->data);
	}

	printf("Tree height: %u\n", (unsigned int)root.height);
	rbtree_dump(&root, stdout);
}

static void test_rbtree_insert(struct rbtree_root *root, int key)
{
	struct rbtree_node *node;

	node = malloc(sizeof(*node));
	if(!node)
		return;

	rbtree_set_key(&node->node, key);
	node->data = "Hello World!";
	rbtree_insert(root, &node->node);
}

static void test_tree_random(void)
{
	struct rbtree_root root;

	memset(&root, 0, sizeof(root));

	test_rbtree_insert(&root, 20);;
	test_rbtree_insert(&root, 10);
	test_rbtree_insert(&root, 30);
	test_rbtree_insert(&root, 40);
	test_rbtree_insert(&root, 27);
	test_rbtree_insert(&root, 25);
	test_rbtree_insert(&root, 28);
	test_rbtree_insert(&root, 26);

	printf("Tree height: %u\n", (unsigned int)root.height);
	rbtree_dump(&root, stdout);
}

int main(int argc, char **argv)
{
	test_tree_increasing();
	putc('\n', stdout);
	test_tree_random();

	return -EXIT_SUCCESS;
}

