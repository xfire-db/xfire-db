/*
 *  Red-black tree node
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

#ifndef __RB_NODE_H__
#define __RB_NODE_H__

#include <xfire/binarytreekey.h>
#include <xfire/binarytreevalue.h>
#include <xfire/binarytreenode.h>

#define RB_READ_LOCK_FLAG  0
#define RB_WRITE_LOCK_FLAG 1
#define RB_COL_FLAG	   2

#define RB_RED true
#define RB_BLACK false

#define rb_node_method(__k, __v, args...) \
	template <typename __k, typename __v> args RBTreeNode<__k,__v>
#define rb_node_constructor(__k, __v) \
	template <typename __k, typename __v> RBTreeNode<__k,__v>
#define rb_node_deconstructor(__k, __v) rb_node_constructor(__k,__v)

template <typename K, typename V> class RBTree;

template <typename K, typename V> class RBTreeNode : public BinaryTreeNode<K,V> {
	public:
		explicit RBTreeNode(K key, V& value);
		virtual ~RBTreeNode();

		void lock();
		void unlock();
		void read_lock();
		void read_unlock();

		RBTreeNode<K,V> *left();
		RBTreeNode<K,V> *right();
		RBTreeNode<K,V> *parent();
		RBTreeNode<K,V> *grandparent();
		RBTreeNode<K,V> *sibling();

		void set_colour(bool c);
		bool colour();
		bool test_and_set_colour(bool c);

		RBTreeNode<K,V> *rotate_right(RBTree<K,V> *tree);
		RBTreeNode<K,V> *rotate_left(RBTree<K,V> *tree);
		void rotate_swap_parent(RBTree<K,V> *tree);

	private:
		unsigned long flags;
};

#include <xfire/bstinstances.h>
#endif

