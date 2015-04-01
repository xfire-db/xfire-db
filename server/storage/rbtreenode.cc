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

#include <stdlib.h>
#include <stdint.h>

#include <xfire/bitops.h>
#include <xfire/rbtreenode.h>
#include <xfire/rbtree.h>

rb_node_constructor(K,V)::RBTreeNode(K key, V& value) :
	BinaryTreeNode<K,V>(key, value)
{
	this->flags = 0UL;
	set_bit(RB_COL_FLAG, &this->flags);
}

rb_node_deconstructor(K,V)::~RBTreeNode()
{
}

rb_node_method(K,V,void)::lock()
{
}

rb_node_method(K,V,void)::unlock()
{
}

rb_node_method(K,V,void)::read_lock()
{
}

rb_node_method(K,V,void)::read_unlock()
{
}

rb_node_method(K,V,void)::set_colour(bool c)
{
	if(c)
		set_bit(RB_COL_FLAG, &this->flags);
	else
		clear_bit(RB_COL_FLAG, &this->flags);
}

rb_node_method(K,V,bool)::colour()
{
	return test_bit(RB_COL_FLAG, &this->flags) != 0;
}

rb_node_method(K,V,bool)::test_and_set_colour(bool c)
{
	bool rv;

	rv = (c) ? !!test_and_set_bit(RB_COL_FLAG, &this->flags) :
		!!test_and_clear_bit(RB_COL_FLAG, &this->flags);
	return rv;
}

rb_node_method(K,V,RBTreeNode<K,V>*)::rotate_left(RBTree<K,V> *tree)
{
	RBTreeNode<K,V> *right,
			*parent;

	right = dynamic_cast<RBTreeNode<K,V>*>(this->right());
	parent = dynamic_cast<RBTreeNode<K,V>*>(this->parent());

	this->set_right(right->right());
	right->set_parent(parent);
	right->set_left(this);
	this->set_parent(right);

	if(this->right())
		this->right()->set_parent(this);

	if(parent) {
		if(this == parent->left())
			parent->set_left(right);
		else
			parent->set_right(right);
	} else {
		tree->set_root(right);
	}

	return right;
}

rb_node_method(K,V,RBTreeNode<K,V>*)::rotate_right(RBTree<K,V> *tree)
{
	RBTreeNode<K,V> *left,
			*parent;

	left = dynamic_cast<RBTreeNode<K,V>*>(this->left());
	parent = dynamic_cast<RBTreeNode<K,V>*>(this->parent());

	this->set_left(left->right());
	left->set_parent(parent);
	this->set_parent(left);
	left->set_right(this);

	if(this->left())
		this->left()->set_parent(this);

	if(parent) {
		if(this == parent->right())
			parent->set_right(left);
		else
			parent->set_left(left);
	} else {
		tree->set_root(left);
	}

	return left;
}

rb_node_method(K,V,void)::rotate_swap_parent(RBTree<K,V> *tree)
{
	RBTreeNode<K,V> *parent = dynamic_cast
		<RBTreeNode<K,V>*>(this->parent());
	bool colour = parent->colour();

	if(parent->right() == this)
		parent->rotate_left(tree);
	else
		parent->rotate_right(tree);

	parent->set_colour(this->colour());
	this->set_colour(colour);
}

rb_node_method(K,V,RBTreeNode<K,V>*)::left()
{
	return dynamic_cast<RBTreeNode<K,V>*>(BinaryTreeNode<K,V>::left());
}

rb_node_method(K,V,RBTreeNode<K,V>*)::right()
{
	return dynamic_cast<RBTreeNode<K,V>*>(BinaryTreeNode<K,V>::right());
}

rb_node_method(K,V,RBTreeNode<K,V>*)::sibling()
{
	RBTreeNode<K,V> *parent;

	parent = this->parent();
	if(parent) {
		if(this == parent->left())
			return parent->right();
		else
			return parent->left();
	}

	return NULL;
}

rb_node_method(K,V,RBTreeNode<K,V>*)::grandparent()
{
	if(this->parent())
		return this->parent()->parent();
	else
		return NULL;
}

rb_node_method(K,V,RBTreeNode<K,V>*)::parent()
{
	return dynamic_cast<RBTreeNode<K,V>*>(BinaryTreeNode<K,V>::parent());
}

template class RBTreeNode<int,std::string>;

