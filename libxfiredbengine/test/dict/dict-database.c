/*
 *  Database unit test
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

#include <xfiredb/xfiredb.h>
#include <xfiredb/types.h>
#include <xfiredb/database.h>
#include <xfiredb/os.h>
#include <xfiredb/container.h>

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

static void *test_thread_a(void *arg)
{
	int i = 0;

	for(; i < 5; i++) {
		assert(db_store(arg, (const char*)dbg_keys[i],
			(void*)dbg_values[i]) == -XFIREDB_OK);
	}
	return NULL;
}

static void *test_thread_b(void *arg)
{
	int i = 5;

	for(; i < 10; i++) {
		assert(db_store(arg, (const char*)dbg_keys[i],
			(void*)dbg_values[i]) == -XFIREDB_OK);
	}
	return NULL;
}

static void *test_thread_c(void *arg)
{
	int i = 10;

	for(; i < 15; i++) {
		assert(db_store(arg, (const char*)dbg_keys[i],
			(void*)dbg_values[i]) == -XFIREDB_OK);
	}
	return NULL;
}

static void *test_thread_e(void *arg)
{
	int i = 20;
	db_data_t data;

	for(; i < 25; i++)
		assert(db_delete(arg, (const char*)dbg_keys[i], &data) == -XFIREDB_OK);
	return NULL;
}

static void *test_thread_d(void *arg)
{
	int i = 15;
	db_data_t data;

	for(; i < 20; i++)
		assert(db_delete(arg, (const char*)dbg_keys[i], &data) == -XFIREDB_OK);
	return NULL;
}

static struct database *strings;

static void setup(struct unit_test *test)
{
	strings = db_alloc("test-db");
}

static void teardown(struct unit_test *test)
{
	db_free(strings);
}

static void test_database(void)
{
	struct thread *a, *b, *c, *d, *e;
	int i = 0;
	db_data_t val, tmp;


	for(i = 15; i < 25; i++)
		assert(db_store(strings, (const char*)dbg_keys[i],
			(void*)dbg_values[i]) == -XFIREDB_OK);

	a = xfiredb_create_thread("thread a", &test_thread_a, strings);
	b = xfiredb_create_thread("thread b", &test_thread_b, strings);
	c = xfiredb_create_thread("thread c", &test_thread_c, strings);
	d = xfiredb_create_thread("thread d", &test_thread_d, strings);
	e = xfiredb_create_thread("thread e", &test_thread_e, strings);

	xfiredb_thread_join(a);
	xfiredb_thread_join(b);
	xfiredb_thread_join(c);
	xfiredb_thread_join(d);
	xfiredb_thread_join(e);

	xfiredb_thread_destroy(a);
	xfiredb_thread_destroy(b);
	xfiredb_thread_destroy(c);
	xfiredb_thread_destroy(d);
	xfiredb_thread_destroy(e);

	for(i = 0; i < 15; i++) {
		if(i == 11)
			continue;
		assert(db_delete(strings, dbg_keys[i], &val) == -XFIREDB_OK);
	}

	assert(db_lookup(strings, dbg_keys[11], &tmp) == -XFIREDB_OK);
	assert(!strcmp(dbg_values[11], tmp.ptr));
	assert(db_delete(strings, dbg_keys[11], &val) == -XFIREDB_OK);
}

static test_func_t test_func_array[] = {test_database, NULL};
struct unit_test dict_database_test = {
	.name = "storage:dict:database",
	.setup = setup,
	.teardown = teardown,
	.tests = test_func_array,
};

