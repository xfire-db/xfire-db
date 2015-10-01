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
#include <xfire/os.h>

static const char *dbg_keys[] = {"key1","key2","key3","key4","key5","key6","key7",
				"key8","key9","key10","key10","key11","key12",
				"key13","key14","key15",
				};

static const char *dbg_values[] = {"val1","val2","val3","val4","val5","val6","val7",
				"val8","val9","val10","val10","val11","val12",
				"val13","val14","val15",
				};

void *test_thread_a(void *arg)
{
	int i = 0;

	printf("Insert thread \"a\" starting!\n");
	for(; i < 5; i++) {
		dict_add(arg, (const char*)dbg_keys[i],
			(void*)dbg_values[i], DICT_PTR);
	}
	return NULL;
}

void *test_thread_b(void *arg)
{
	int i = 5;

	printf("Insert thread \"b\" starting!\n");
	for(; i < 10; i++) {
		dict_add(arg, (const char*)dbg_keys[i],
			(void*)dbg_values[i], DICT_PTR);
	}
	return NULL;
}

void *test_thread_c(void *arg)
{
	int i = 10;

	printf("Insert thread \"c\" starting!\n");
	for(; i < 15; i++) {
		dict_add(arg, (const char*)dbg_keys[i],
			(void*)dbg_values[i], DICT_PTR);
	}
	return NULL;
}

int main(int argc, char **argv)
{
	struct dict *strings;
	struct thread *a, *b, *c;
	int i = 0;
	char *tmp = NULL;

	strings = dict_alloc();

	a = xfire_create_thread("thread a", &test_thread_a, strings);
	b = xfire_create_thread("thread b", &test_thread_b, strings);
	c = xfire_create_thread("thread c", &test_thread_c, strings);

	xfire_thread_join(a);
	xfire_thread_join(b);
	xfire_thread_join(c);

	xfire_thread_destroy(a);
	xfire_thread_destroy(b);
	xfire_thread_destroy(c);

	for(; i < 15; i++) {
		if(i == 11)
			continue;
		dict_delete(strings, dbg_keys[i], false);
	}

	printf("Testing lookup...\n");
	dict_lookup(strings, dbg_keys[11], &tmp, DICT_PTR);
	if(tmp)
		printf("Found %s under key %s!\n", tmp, dbg_keys[11]);
	dict_delete(strings, dbg_keys[11], false);

	dict_free(strings);
	return -EXIT_SUCCESS;
}
