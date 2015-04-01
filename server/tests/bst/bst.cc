/*
 *  BST test
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
#include <string.h>
#include <stdio.h>

#include <xfire/testdatabase.h>

int main(int argc, char **argv)
{
	BSTDatabase *db = new BSTDatabase();
	std::string data("Hello World");
	std::string rv;

	db->insert(10, data);
	rv = db->find(10);

	printf("Found value 10::%s\n", rv.c_str());

	delete db;
	return -EXIT_SUCCESS;
}

