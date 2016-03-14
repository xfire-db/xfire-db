/*
 *  Disk unit test
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
#include <xfiredb/log.h>
#include <xfiredb/types.h>
#include <xfiredb/mem.h>
#include <xfiredb/disk.h>
#include <xfiredb/string.h>

static struct string s1, s2, s3, s4;

static void test_hm_insert(struct hashmap *map)
{
	string_init(&s1);
	string_init(&s2);
	string_init(&s3);
	string_init(&s4);

	string_set(&s1, "test-val-1");
	string_set(&s2, "test-val-2");
	string_set(&s3, "test-val-3");
	string_set(&s4, "test-val-4");

	hashmap_add(map, "key1", &s1.node);
	hashmap_add(map, "key2", &s2.node);
	hashmap_add(map, "key3", &s3.node);
	hashmap_add(map, "key4", &s4.node);
}

static struct string *dbg_get_string(const char *c)
{
	struct string *s = xfiredb_zalloc(sizeof(*s));

	string_init(s);
	string_set(s, c);
	return s;
}

static void dbg_hm_clear(struct hashmap *map)
{
	struct hashmap_node *hnode;
	struct hashmap_iterator *hit;
	struct string *s;

	hit = hashmap_new_iterator(map);
	while((hnode = hashmap_iterator_next(hit)) != NULL) {
		s = container_of(hnode, struct string, node);
		hashmap_iterator_delete(hit);
		hashmap_node_destroy(hnode);
		string_destroy(s);
	}
	hashmap_free_iterator(hit);
}

static void dbg_hm_store(struct disk *d)
{
	struct hashmap map;

	hashmap_init(&map);
	test_hm_insert(&map);
	disk_store_hm(d, "hm-key", &map);
	disk_update_hm(d, "hm-key", "key3", "hm-update-ok");
	disk_delete_hashmapnode(d, "hm-key", "key2");
	dbg_hm_clear(&map);
	hashmap_destroy(&map);
}

static void dbg_list_store(struct disk *d)
{
	struct string *s1, *s2, *s3, *s4;
	struct list_head lh;

	list_head_init(&lh);
	s1 = dbg_get_string("entry-1");
	s2 = dbg_get_string("entry-2");
	s3 = dbg_get_string("entry-3");
	s4 = dbg_get_string("entry-3");

	list_rpush(&lh, &s1->entry);
	list_rpush(&lh, &s2->entry);
	list_rpush(&lh, &s3->entry);
	list_rpush(&lh, &s4->entry);

	disk_store_list(d, "list-key", &lh);
	disk_delete_list(d, "list-key", "entry-3");
	disk_update_list(d, "list-key", "entry-3", "entry-4");

	list_del(&lh, &s1->entry);
	list_del(&lh, &s2->entry);
	list_del(&lh, &s3->entry);
	list_del(&lh, &s4->entry);

	string_destroy(s1);
	string_destroy(s2);
	string_destroy(s3);
	string_destroy(s4);

	xfiredb_free(s1);
	xfiredb_free(s2);
	xfiredb_free(s3);
	xfiredb_free(s4);
}

static void setup(struct unit_test *t)
{
	xfiredb_log_init(NULL, NULL);
}

static void teardown(struct unit_test *t)
{
	xfiredb_log_exit();
}

static void disk_test(void)
{
	struct disk *d;
	struct string *s;

	d = disk_create(SQLITE_DB);
	s = dbg_get_string("test-data");
	assert(!disk_store_string(d, "test-key", s->str));

	disk_update_string(d, "test-key", "String update success!");

	string_destroy(s);
	xfiredb_free(s);

	dbg_list_store(d);
	dbg_hm_store(d);
	disk_dump(d, stdout);
	disk_destroy(d);
}

static test_func_t test_func_array[] = {disk_test, NULL};
struct unit_test disk_single_test = {
	.name = "storage:disk",
	.setup = setup,
	.teardown = teardown,
	.tests = test_func_array,
};

