/*
 *  Binary search tree node
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

template <typename K, typename V> BinaryTreeNode<K,V>::BinaryTreeNode(
			K& key, V& value)
{
	this->key = key;
	this->value = value;
}

template <typename K, typename V> BinaryTreeNode<K,V>::BinaryTreeNode()
{
}

template <typename K, typename V> BinaryTreeNode<K,V>::~BinaryTreeNode()
{
}


template <typename K, typename V> K& BinaryTreeNode<K,V>::get_key()
{
	return this->key;
}

template <typename K, typename V> V& BinaryTreeNode<K,V>::get_value()
{
	return this->value;
}

/* setters */

template <typename K, typename V> void 
BinaryTreeNode<K,V>::set_left(BinaryTreeNode<K,V> *node)
{
	this->_left = node;
}

template <typename K, typename V> void 
BinaryTreeNode<K,V>::set_right(BinaryTreeNode<K,V> *node)
{
	this->_right = node;
}

template <typename K, typename V> void
BinaryTreeNode<K,V>::set_parent(BinaryTreeNode<K,V> *node)
{
	this->_parent = node;
}

/* getters */

template <typename K, typename V> BinaryTreeNode<K,V> *
BinaryTreeNode<K,V>::left()
{
	return this->_left;
}

template <typename K, typename V> BinaryTreeNode<K,V> *
BinaryTreeNode<K,V>::right()
{
	return this->_right;
}

template <typename K, typename V> BinaryTreeNode<K,V> *
BinaryTreeNode<K,V>::sibling()
{
	BinaryTreeNode<K,V> *parent;

	parent = this->parent();
	if(parent) {
		if(this == parent->left())
			return parent->right();
		else
			return parent->left();
	}

	return NULL;
}

template <typename K, typename V> BinaryTreeNode<K,V> *
BinaryTreeNode<K,V>::grandparent()
{
	if(this->_parent)
		return this->_parent->parent();
	else
		return NULL;
}

template <typename K, typename V> BinaryTreeNode<K,V> *
BinaryTreeNode<K,V>::parent()
{
	return this->_parent;
}

#ifdef HAVE_DBG
template class BinaryTreeNode<int,std::string>;
#endif

