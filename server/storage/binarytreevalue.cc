/*
 *  Binary search tree value
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

#include <xfire/binarytreevalue.h>

template <typename V> BinaryTreeValue<V>::BinaryTreeValue(V value)
{
	this->value = value;
}

template <typename V> BinaryTreeValue<V>::~BinaryTreeValue()
{
}

template <typename V> V BinaryTreeValue<V>::get_value()
{
	return this->value;
}
