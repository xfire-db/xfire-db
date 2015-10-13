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

#include <sys/time.h>

#include <xfire/xfire.h>
#include <xfire/types.h>
#include <xfire/mem.h>
#include <xfire/disk.h>
#include <xfire/string.h>

static struct string *dbg_get_string(const char *c)
{
	struct string *s = xfire_zalloc(sizeof(*s));

	string_init(s);
	string_set(s, c);
	return s;
}

static void dbg_list_store(struct disk *d)
{
	struct string *s1, *s2, *s3;
	struct list_head lh;

	list_head_init(&lh);
	s1 = dbg_get_string("entry-1");
	s2 = dbg_get_string("entry-2");
	s3 = dbg_get_string("entry-3");

	list_rpush(&lh, &s1->entry);
	list_rpush(&lh, &s2->entry);
	list_rpush(&lh, &s3->entry);

	disk_store_list(d, "list-key", &lh);
	disk_lookup(d, "list-key");

	list_del(&lh, &s1->entry);
	list_del(&lh, &s2->entry);
	list_del(&lh, &s3->entry);

	string_destroy(s1);
	string_destroy(s2);
	string_destroy(s3);

	xfire_free(s1);
	xfire_free(s2);
	xfire_free(s3);
}

int main(int argc, char **argv)
{
	struct disk *d;
	char *lookup;
	struct string *s;

	d = disk_create(SQLITE_DB);
	s = dbg_get_string("test-data");
	if(!disk_store_string(d, "test-key", s))
		fprintf(stdout, "Key store succesfull!\n");

	/*if(!disk_update(d, "test-key", "test-data-update", sizeof("test-data-update")))
		fprintf(stdout, "Key update succesfull!\n");*/

	lookup = disk_lookup(d, "test-key");
	if(lookup)
		printf("%s = %s\n", "test-key", lookup);

	disk_result_free(lookup);
	string_destroy(s);
	xfire_free(s);

	dbg_list_store(d);
	disk_destroy(d);
	return -EXIT_SUCCESS;
}

