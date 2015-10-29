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
#include <unittest.h>

#include <sys/time.h>

#include <xfiredb/engine/xfiredb.h>
#include <xfiredb/engine/types.h>
#include <xfiredb/engine/dict.h>
#include <xfiredb/engine/os.h>

static const char *dbg_keys[] = {"key1","key2","key3","key4","key5","key6","key7",
				"key8","key9","key10","key11","key12",
				"key13","key14","key15","pre1","pre2","pre3","pre4",
				"pre5","pre6","pre7","pre8","pre9","pre10",
				};

static const char *dbg_values[] = {"val1","val2","val3","val4","val5","val6","val7",
				"val8","val9","val10","val11","val12",
				"val13","val14","val15","preval1","preval2","preval3","preval4",
				"preval5","preval6","preval7","preval8","preval9","preval10",
				};

void *test_thread_a(void *arg)
{
	int i = 0;
	int rc;

	for(; i < 5; i++) {
		rc = strlen((char*)dbg_values[i]);
		raw_dict_add(arg, (const char*)dbg_keys[i],
			(void*)dbg_values[i], DICT_PTR, rc);
	}
	return NULL;
}

void *test_thread_b(void *arg)
{
	int i = 5;

	for(; i < 10; i++) {
		dict_add(arg, (const char*)dbg_keys[i],
			(void*)dbg_values[i], DICT_PTR);
	}
	return NULL;
}

void *test_thread_c(void *arg)
{
	int i = 10;

	for(; i < 15; i++) {
		dict_add(arg, (const char*)dbg_keys[i],
			(void*)dbg_values[i], DICT_PTR);
	}
	return NULL;
}

void *test_thread_e(void *arg)
{
	int i = 20;
	union entry_data val;

	for(; i < 25; i++)
		dict_delete(arg, (const char*)dbg_keys[i], &val,
			false);
	return NULL;
}

void *test_thread_d(void *arg)
{
	int i = 15;
	union entry_data val;

	for(; i < 20; i++)
		dict_delete(arg, (const char*)dbg_keys[i], &val,
			false);
	return NULL;
}

static struct dict *strings;

void setup(void)
{
	struct thread *a, *b, *c, *d, *e;
	int i = 0;

	strings = dict_alloc();

	for(i = 15; i < 25; i++)
		dict_add(strings, (const char*)dbg_keys[i],
			(void*)dbg_values[i], DICT_PTR);

	a = xfire_create_thread("thread a", &test_thread_a, strings);
	b = xfire_create_thread("thread b", &test_thread_b, strings);
	c = xfire_create_thread("thread c", &test_thread_c, strings);
	d = xfire_create_thread("thread d", &test_thread_d, strings);
	e = xfire_create_thread("thread e", &test_thread_e, strings);

	xfire_thread_join(a);
	xfire_thread_join(b);
	xfire_thread_join(c);
	xfire_thread_join(d);
	xfire_thread_join(e);

	xfire_thread_destroy(a);
	xfire_thread_destroy(b);
	xfire_thread_destroy(c);
	xfire_thread_destroy(d);
	xfire_thread_destroy(e);
}

void teardown(void)
{
	dict_free(strings);
}

void test_dict_conncurrent(void)
{
	int rv;
	int i;
	size_t size;
	union entry_data tmp, val;

	for(i = 0; i < 15; i++) {
		if(i == 11)
			continue;
		rv = dict_delete(strings, dbg_keys[i], &val, false);
		assert(rv == -XFIRE_OK);
	}

	dict_lookup(strings, dbg_keys[11], &tmp, &size);
	assert(!strcmp(dbg_values[11], tmp.ptr));
	assert(dict_delete(strings, dbg_keys[11], &val, false) == -XFIRE_OK);
}

test_func_t test_func_array[] = {test_dict_conncurrent, NULL};
const char *test_name = "Concurrent dictionary test";

