/*
 *  Binary search tree
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

#include <xfire/xfire.h>
#include <xfire/binarytreenode.h>
#include <xfire/binarytree.h>


template <typename K, typename V> BinaryTree<K,V>::BinaryTree()
{
	this->_root = NULL;
}

template <typename K, typename V> BinaryTree<K,V>::~BinaryTree()
{
	if(this->_root)
		delete this->_root;
}

template <typename K, typename V> BinaryTreeNode<K,V> *BinaryTree<K,V>::root()
{
	return this->_root;
}

/*
 * Insert support
 */

template <typename K, typename V> void BinaryTree<K,V>::insert_key(
		BinaryTreeNode<K,V> *node
		)
{
	BinaryTreeNode<K,V> *iterator = this->root();

	for(;;) {
		if(node->get_key() <= iterator->get_key()) {
			if(iterator->left() == NULL) {
				iterator->set_left(node);
				node->set_parent(iterator);
				break;
			}

			iterator = iterator->left();
			continue;
		} else {
			if(iterator->right() == NULL) {
				iterator->set_right(node);
				node->set_parent(iterator);
			}

			iterator = iterator->right();
			continue;
		}
	}
}

template <typename K, typename V> void BinaryTree<K,V>::insert(
		BinaryTreeNode<K,V> *node
		)
{
	if(!this->_root)
		this->_root = node;

	this->insert_key(node);
}


template <typename K, typename V> void BinaryTree<K,V>::insert(K key, V& val)
{
	BinaryTreeNode<K,V> *node = new BinaryTreeNode<K,V>(key, val);

	this->insert(node);
}

/*
 * Search support
 */
template <typename K, typename V> BinaryTreeNode<K,V> *BinaryTree<K,V>::find(
		K& key
		)
{
	return this->find(this->root(), key);
}

template <typename K, typename V> BinaryTreeNode<K,V> *BinaryTree<K,V>::find(
		BinaryTreeNode<K,V> *node,
		K& key
		)
{
	if(!node)
		return NULL;

	if(node->get_key() == key)
		return node;

	if(key < node->get_key())
		return this->find(node->left(), key);
	else
		return this->find(node->right(), key);
}

template <typename K, typename V> void BinaryTree<K,V>::set_root(
		BinaryTreeNode<K,V> *node
		)
{
	this->_root = node;
}

#ifdef HAVE_DBG
template class BinaryTree<int,std::string>;
#endif

