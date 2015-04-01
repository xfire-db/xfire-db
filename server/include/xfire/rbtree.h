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

#ifndef __RBTREE_H__
#define __RBTREE_H__

#include <xfire/rbtreenode.h>
#include <xfire/binarytree.h>

#define rb_method(__k, __v, args...) \
	template <typename __k, typename __v> args RBTree<__k,__v>
#define rb_constructor(__k, __v) \
	template <typename __k, typename __v> RBTree<__k,__v>
#define rb_deconstructor(__k, __v) rb_constructor(__k,__v)

template <typename K, typename V> class RBTree : public BinaryTree<K,V> {
	public:
	explicit RBTree();
	virtual ~RBTree();

	RBTreeNode<K,V> *root();

	int insert(RBTreeNode<K,V> *node);
	int insert(K key, V& value);
	RBTreeNode<K,V> *remove(K key);

	protected:
	RBTreeNode<K,V> *remove(RBTreeNode<K,V> *node);
	unsigned long nodes;

	private:
	double balance(RBTreeNode<K,V> *node);
};

#include <xfire/bstinstances.h>
#endif

