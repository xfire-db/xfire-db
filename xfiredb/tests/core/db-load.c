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
	char *d1, *d2, *d3;

	xfiredb_string_get("key1", &d1);
	xfiredb_string_get("key2", &d2);
	xfiredb_string_get("key3", &d3);

	fprintf(stdout, "Found strings:\n* %s\n* %s\n* %s\n", d1, d2, d3);
	xfire_free(d1);
	xfire_free(d2);
	xfire_free(d3);

	xfiredb_key_delete("key1");
	xfiredb_key_delete("key2");
	xfiredb_key_delete("key3");
}

static void dbg_list_test(void)
{
	int idx[] = {0,1,2,3};
	int num = 4, i;
	char **data = xfire_zalloc(sizeof(*data) * num);

	xfiredb_list_get("key4", data, idx, num);
	printf("List entry's found (%i):\n* %s\n* %s\n* %s\n* %s\n",
			xfiredb_list_length("key4"),
			data[0], data[1], data[2], data[3]);

	printf("List entry's deleted: %i\n",
			xfiredb_list_pop("key4", idx, num));

	for(i = 0; i < num; i++)
		xfire_free(data[i]);
	xfire_free(data);
}

static void dbg_hm_test(void)
{
	char *keys[] = {"skey1", "skey2",};
	char *rmkeys[] = {"skey1","skey2","skey3"};
	int num = 2, i;
	char **data = xfire_zalloc(sizeof(*data) * 2);

	xfiredb_hashmap_get("key5", keys, data, 2);
	printf("Hashmap entry's:\n* %s\n* %s\n", data[0], data[1]);

	xfiredb_hashmap_remove("key5", rmkeys, 3);

	for(i = 0; i < num; i++)
		xfire_free(data[i]);
	xfire_free(data);
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

