/*
 *  Red-black tree
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
#include <math.h>

#include <xfire/xfire.h>
#include <xfire/binarytree.h>
#include <xfire/rbtreenode.h>
#include <xfire/rbtree.h>

rb_constructor(K,V)::RBTree()
	: BinaryTree<K,V>()
{
}

rb_deconstructor(K,V)::~RBTree()
{
}

rb_method(K,V,RBTreeNode<K,V>*)::root()
{
	return dynamic_cast<RBTreeNode<K,V>*>(BinaryTree<K,V>::root());
}

rb_method(K,V,int)::insert(K key, V& value)
{
	RBTreeNode<K,V> *node = new RBTreeNode<K,V>(key, value);
	return this->insert(node);
}

rb_method(K,V,int)::insert(RBTreeNode<K,V> *node)
{
	node->set_colour(RB_RED);
	if(this->find(node->get_key()))
		return -EXIT_FAILURE;
	else
		BinaryTree<K,V>::insert(node);

	this->_height = this->balance(node);
	return -EXIT_SUCCESS;
}

rb_method(K,V,RBTreeNode<K,V>*)::remove(K key)
{
	return NULL;
}

rb_method(K,V,RBTreeNode<K,V>*)::remove(RBTreeNode<K,V> *node)
{
	return NULL;
}

rb_method(K,V,double)::balance(RBTreeNode<K,V> *node)
{
	RBTreeNode<K,V> *sibling;

	if(node == this->root())
		return this->_height;

	node = node->parent();
	while(node != this->root() && node->colour() == RB_RED) {
		sibling = node->sibling();
		if(sibling) {
			/*
			 * pull black down from the GP
			 */
			if(sibling->test_and_set_colour(RB_BLACK)) {
				node->set_colour(RB_BLACK);
				node->parent()->set_colour(RB_RED);
				
				node = node->grandparent();
				if(!node)
					break;

				continue;
			} else {
				/*
				 * rotate into the direction of node's
				 * relation with node's parent
				 */
				if(node->parent()->left() == node &&
						node->left()->colour()) {
					node->rotate_right(this);
					node = node->parent();
				} else if(node->parent()->left() != node &&
						!node->left()->colour()) {
					node->rotate_left(this);
					node = node->parent();
				}

				node->rotate_swap_parent(this);
			}
		} else {
			node->rotate_swap_parent(this);
		}
	}

	this->root()->set_colour(RB_BLACK);
	this->nodes += 1;
	return (log10(this->nodes) / log10(2));
}

#ifdef HAVE_DBG
template class RBTree<int,std::string>;
#endif

