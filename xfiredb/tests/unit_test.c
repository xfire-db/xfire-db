/*
 *  XFireDB unit testing frame work
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
#include <stdio.h>
#include <unittest.h>

extern test_func_t test_func_array[];
extern const char *test_name;
extern void setup();
extern void teardown();

int main(int argc, char **argv)
{
	test_func_t hook;
	int i;

	printf("Starting unit test: %s\n", test_name);
	for(i = 0;; i++) {
		hook = test_func_array[i];
		printf("[case %i]: starting\n",i+1);
		setup();
		hook();
		teardown();
		printf("[case %i]: ended\n",i+1);

		if(test_func_array[i+1] == NULL)
			break;
	}

	printf("Unit test ended sucessfully!\n");
	return -EXIT_SUCCESS;
}

