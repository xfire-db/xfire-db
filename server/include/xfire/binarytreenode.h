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

#ifndef __BST_NODE_H__
#define __BST_NODE_H__

#include <xfire/binarytreekey.h>
#include <xfire/binarytreevalue.h>

template <typename K, typename V> class BinaryTreeNode {
	public:
		explicit BinaryTreeNode(BinaryTreeKey<K> &key, 
				BinaryTreeValue<V> &value);
		explicit BinaryTreeNode();
		virtual ~BinaryTreeNode();

		void set_value(BinaryTreeValue<V> &value);

		BinaryTreeKey<K> &get_key();
		BinaryTreeValue<V> &get_value();

		BinaryTreeNode<K,V> *left();
		BinaryTreeNode<K,V> *right();
		BinaryTreeNode<K,V> *parent();

		void set_left(BinaryTreeNode<K,V> *node);
		void set_parent(BinaryTreeNode<K,V> *node);
		void set_right(BinaryTreeNode<K,V> *node);

	private:
		BinaryTreeKey<K> key;
		BinaryTreeValue<V> value;

		BinaryTreeNode<K,V> *_parent;
		BinaryTreeNode<K,V> *_left;
		BinaryTreeNode<K,V> *_right;
};
#endif
