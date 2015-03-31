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

#ifndef __BINARY_TREE_H_
#define __BINARY_TREE_H_

#include <xfire/binarytreenode.h>

template <class K, class V> class BinaryTree {
	public:
		explicit BinaryTree();
		virtual ~BinaryTree();

		BinaryTreeNode<K,V> *root();
		void insert(BinaryTreeNode<K,V> *node);
		void insert(K& v, V& val);

		virtual BinaryTreeNode<K,V> *remove(K& key) = 0;
		virtual BinaryTreeNode<K,V> *remove(BinaryTreeNode<K,V> *node) = 0;

		BinaryTreeNode<K,V> *find(K& key);

	private:
		void insert_key(BinaryTreeNode<K,V> *node);
		BinaryTreeNode<K,V> *find(BinaryTreeNode<K,V> *node, K& key);

		BinaryTreeNode<K,V> *_root;
};

#endif

