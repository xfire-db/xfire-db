/*
 *  Base database
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

#ifndef __TEST_DB__
#define __TEST_DB__

#include <string.h>

#include <xfire/database.h>
#include <xfire/binarytree.h>

template <typename K, typename V> class TestBST : public BinaryTree<K,V> {
	public:
	explicit TestBST(){}
	virtual ~TestBST(){}

	void insert(K v, V& val)
	{
		BinaryTree<K,V>::insert(v, val);
	}

	BinaryTreeNode<K,V> *remove(K& key)
	{
		return NULL;
	}

	protected:
	BinaryTreeNode<K,V> *remove(BinaryTreeNode<K,V> *node)
	{
		return NULL;
	}

};

class BSTDatabase : public Database<int,std::string> {
	public:
		explicit BSTDatabase();
		virtual ~BSTDatabase();

		void insert(int key, std::string& value);
		void remove(int key);
		std::string& find(int key);

	private:
		TestBST<int,std::string> *tree;
};

#endif
