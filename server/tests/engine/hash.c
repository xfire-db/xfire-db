/*
 *  XFIRE HASHING
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

#include <xfire/xfire.h>
#include <xfire/types.h>
#include <xfire/hash.h>

int main(int argc, char **argv)
{
	u64 hash;
	int i = 1;

	if(argc < 2)
		fprintf(stderr, "Usage: %s <key> ..\n", argv[0]);

	for(; i < argc; i++) {
		xfire_hash(argv[i], &hash);
		printf("Hashing value of key '%s\': %llu\n", argv[i],
				(unsigned long long)hash);
	}

	return -EXIT_SUCCESS;
}
