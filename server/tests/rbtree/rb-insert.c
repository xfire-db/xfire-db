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

struct data_node {
	struct rb_node node;
	const char *data;
};

static bool compare_node(struct rb_node *node, const void *arg)
{

	struct data_node *container;

	container = container_of(node, struct data_node, node);
	if(!strcmp(container->data, arg))
		return true;

	return false;
}

/*static void test_tree_increasing(void)
{
	struct rb_root root;
	struct data_node *node;
	struct rb_node *find;
	int i = 1;

	memset(&root, 0, sizeof(root));
	root.iterate = &compare_node;
	rb_init_root(&root);

	for(; i <= 9; i++) {
		node = malloc(sizeof(*node));
		rb_init_node(&node->node);
		rb_set_key(&node->node, i);
		node->data = "Hello World!";
		rb_insert_duplicate(&root, &node->node);

		if(i == 7) {
			node = malloc(sizeof(*node));
			rb_init_node(&node->node);
			rb_set_key(&node->node, i);
			node->data = "Hello World 2!";
			rb_insert_duplicate(&root, &node->node);

			node = malloc(sizeof(*node));
			rb_init_node(&node->node);
			rb_set_key(&node->node, i);
			node->data = "Hello World 3!";
			rb_insert_duplicate(&root, &node->node);
		}
	}

	rb_remove(&root, 5, "Hello World!");
	rb_remove(&root, 7, "Hello World!");

	find = rb_find_duplicate(&root, 7, &compare_node, "Hello World 3!");

	if(find) {
		printf("[OK] Duplicate has been replaced!\n");
		if(find->parent->key == 6ULL)
			printf("[OK] RBTree structure OK\n");
	}


	printf("Tree height: %u\n", (unsigned int)root.height);
	rb_dump(&root, stdout);
}*/

static void test_rb_insert(struct rb_root *root, int key)
{
	struct data_node *node;

	node = malloc(sizeof(*node));
	if(!node)
		return;

	rb_init_node(&node->node);
	rb_set_key(&node->node, key);
	node->data = "Hello World!";
	rb_insert(root, &node->node, false);
}

static void test_incremental(void)
{
	struct rb_root root;
	int i;

	memset(&root, 0, sizeof(root));
	rb_init_root(&root);
	root.iterate = &compare_node;

	for(i = 1; i <= 9; i++)
		test_rb_insert(&root, i);

	rb_remove(&root, 6, "Hello World!");

	rb_dump(&root, stdout);
}

static void test_tree_random(void)
{
	struct rb_root root;

	memset(&root, 0, sizeof(root));
	rb_init_root(&root);

	test_rb_insert(&root, 20);;
	test_rb_insert(&root, 10);
	test_rb_insert(&root, 30);
	test_rb_insert(&root, 40);
	test_rb_insert(&root, 27);
	test_rb_insert(&root, 25);
	test_rb_insert(&root, 28);
	test_rb_insert(&root, 26);

	//rb_remove(&root, 27, "Hello World!");

	/*if(root.tree->key == 26ULL)
		printf("[OK] Root removal OK\n");*/

	rb_dump(&root, stdout);
}

int main(int argc, char **argv)
{
	test_incremental();
	//test_tree_increasing();
	putc('\n', stdout);
	test_tree_random();

	return -EXIT_SUCCESS;
}

