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

#include <sys/time.h>

#include <xfiredb/engine/xfiredb.h>
#include <xfiredb/engine/types.h>
#include <xfiredb/engine/mem.h>
#include <xfiredb/engine/bio.h>
#include <xfiredb/engine/error.h>

static void dbg_string_test(void)
{
	xfiredb_string_set("key1", "string1");
	xfiredb_string_set("key2", "string2");
	xfiredb_string_set("key3", "string3");
}

static void dbg_list_test(void)
{
	xfiredb_list_set("key4", 1, "list-entry-2-updated");
	xfiredb_list_push("key4", "list-entry1", true);
	xfiredb_list_push("key4", "list-entry2", true);
	xfiredb_list_push("key4", "list-entry3", true);
}

static void dbg_hm_test(void)
{
	xfiredb_hashmap_set("key5", "skey1", "hash entry 1");
	xfiredb_hashmap_set("key5", "skey2", "hash entry 2");
	xfiredb_hashmap_set("key5", "skey3", "hash entry 3");
	xfiredb_hashmap_set("key5", "skey2", "hash entry 2, updated");
}

int main(int argc, char **argv)
{
	xfiredb_init();
	dbg_string_test();
	dbg_list_test();
	dbg_hm_test();
	sleep(1);
	bio_sync();
	sleep(1);
	xfiredb_exit();

	return -EXIT_SUCCESS;
}

