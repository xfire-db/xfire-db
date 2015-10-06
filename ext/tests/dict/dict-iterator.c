/*
 *  Hashed dictionary unit test
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

#include <sys/time.h>

#include <xfire/xfire.h>
#include <xfire/types.h>
#include <xfire/dict.h>


static const char *dbg_keys[] = {"key1","key2","key3","key4","key5","key6","key7",
				"key8","key9","key10","key11","key12",
				};

static char *dbg_values[] = {"val1","val2","val3","val4","val5","val6","val7",
				"val8","val9","val10","val11","val12",
				};

static void dot()
{
	fputs(".\n", stdout);
}

static void dbg_setup_dict(struct dict *d)
{
	int i;

	printf("Setting up dictionary\n");
	dot();dot();dot();

	for(i = 0; i < 12; i++) {
		dict_add(d, dbg_keys[i], dbg_values[i],
				DICT_PTR);
	}
}

int main(int argc, char **argv)
{
	struct dict *strings;
	struct dict_iterator *it;
	struct dict_entry *e;
	int i = 0;

	strings = dict_alloc();
	dbg_setup_dict(strings);

	printf("Creating iterator\n");
	it = dict_get_safe_iterator(strings);
	e = dict_iterator_next(it);

	for(; e; e = dict_iterator_next(it)) {
		printf("Found key: %s\n", e->key);
		i++;
	}

	dict_iterator_free(it);

	printf("\nIterating backwards\n");
	it = dict_get_safe_iterator(strings);
	e = dict_iterator_prev(it);
	for(; e; e = dict_iterator_prev(it)) {
		printf("Found key: %s\n", e->key);
	}

	dict_iterator_free(it);
	dict_clear(strings);
	dict_free(strings);

	if(i == 12)
		printf("Done iterating, test succesful!\n");
	else
		fprintf(stderr, "Iteration test failed. " \
			"Number of iterations incorrect!\n");
	return -EXIT_SUCCESS;
}

