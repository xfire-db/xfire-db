/*
 *  XFireDB core test
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
#include <unistd.h>
#include <unittest.h>

#include <sys/time.h>

#include <xfiredb/xfiredb.h>
#include <xfiredb/types.h>
#include <xfiredb/mem.h>
#include <xfiredb/bio.h>
#include <xfiredb/error.h>

static void dbg_string_test(void)
{
	char *d1, *d2, *d3;

	assert(xfiredb_string_set("key1", "string1") == -XFIREDB_OK);
	assert(xfiredb_string_set("key2", "string2") == -XFIREDB_OK);
	assert(xfiredb_string_set("key3", "string3") == -XFIREDB_OK);

	assert(xfiredb_string_get("key1", &d1) == -XFIREDB_OK);
	assert(xfiredb_string_get("key2", &d2) == -XFIREDB_OK);
	assert(xfiredb_string_get("key3", &d3) == -XFIREDB_OK);

	xfiredb_free(d1);
	xfiredb_free(d2);
	xfiredb_free(d3);

	assert(xfiredb_key_delete("key1") == 1);
	assert(xfiredb_key_delete("key2") == 1);
	assert(xfiredb_key_delete("key3") == 1);
}

static void dbg_list_test(void)
{
	int idx[] = {0,1,2,3};
	int num = 4, i;
	char **data = xfiredb_zalloc(sizeof(*data) * num);

	assert(xfiredb_list_set("key4", 1, "list-entry-2-updated") == -XFIREDB_OK);
	assert(xfiredb_list_push("key4", "list-entry1", true) == -XFIREDB_OK);
	assert(xfiredb_list_push("key4", "list-entry2", true) == -XFIREDB_OK);
	assert(xfiredb_list_push("key4", "list-entry3", true) == -XFIREDB_OK);
	
	assert(xfiredb_list_get("key4", data, idx, num) == -XFIREDB_OK);
	assert(xfiredb_list_pop("key4", idx, num) == 4);

	for(i = 0; i < num; i++)
		xfiredb_free(data[i]);
	xfiredb_free(data);
}

static void dbg_hm_test(void)
{
	char *keys[] = {"skey1", "skey2",};
	char *rmkeys[] = {"skey1","skey2","skey3"};
	int num = 2, i;
	char **data = xfiredb_zalloc(sizeof(*data) * 2);

	assert(xfiredb_hashmap_set("key5", "skey1", "hash entry 1") == -XFIREDB_OK);
	assert(xfiredb_hashmap_set("key5", "skey2", "hash entry 2") == -XFIREDB_OK);
	assert(xfiredb_hashmap_set("key5", "skey3", "hash entry 3") == -XFIREDB_OK);
	assert(xfiredb_hashmap_set("key5", "skey2", "hash entry 2, updated") == -XFIREDB_OK);

	assert(xfiredb_hashmap_get("key5", keys, data, 2) == -XFIREDB_OK);
	assert(xfiredb_hashmap_remove("key5", rmkeys, 3) == 3);

	for(i = 0; i < num; i++)
		xfiredb_free(data[i]);
	xfiredb_free(data);
}

void setup(struct unit_test *test)
{
	xfiredb_init();
}

void teardown(struct unit_test *test)
{
	xfiredb_exit();
}

void test_storage_engine(void)
{
	dbg_string_test();
	dbg_list_test();
	dbg_hm_test();
	bio_sync();
	sleep(1);
}

static test_func_t test_func_array[] = {test_storage_engine, NULL};
struct unit_test core_xfiredb_test = {
	.name = "core:xfiredb",
	.setup = setup,
	.teardown = teardown,
	.tests = test_func_array,
};

