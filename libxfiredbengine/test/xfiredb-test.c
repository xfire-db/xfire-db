/*
 *  XFireDB unit testing
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
#include <getopt.h>
#include <unistd.h>
#include <unittest.h>
#include <assert.h>

#include <xfiredb/xfiredb.h>
#include <xfiredb/types.h>
#include <xfiredb/dict.h>

extern struct unit_test dict_single_test;
extern struct unit_test dict_concurrent_test;
extern struct unit_test dict_iterator_test;
extern struct unit_test dict_database_test;

extern struct unit_test core_bitops_test;
extern struct unit_test core_xfiredb_test;
extern struct unit_test core_quotearg_test;
extern struct unit_test core_sleep_test;

extern struct unit_test rb_single_test;
extern struct unit_test rb_concurrent_test;
extern struct unit_test rb_set_test;
extern struct unit_test rb_hashmap_test;

extern struct unit_test disk_single_test;

extern struct unit_test bg_test;
extern struct unit_test bio_test;

static struct unit_test *tests[] = {
	&dict_single_test,
	&dict_concurrent_test,
	&dict_database_test,
	&dict_iterator_test,

	&core_bitops_test,
	&core_xfiredb_test,
	&core_quotearg_test,
	&core_sleep_test,

	&disk_single_test,

	&bio_test,
	&bg_test,

	&rb_single_test,
	&rb_concurrent_test,
	&rb_hashmap_test,
	&rb_set_test,
	NULL,
};

static void test_setup(struct dict *utests)
{
	struct unit_test *t;
	int i = 0, check;

	t = tests[i];
	while(t) {
		check = dict_add(utests, t->name, t, DICT_PTR);
		assert(check == -XFIRE_OK);
		t = tests[++i];
	}
}

static inline void test_exec(struct unit_test *t)
{
	test_func_t hook;
	int i = 0;

	hook = t->tests[0];
	for(; hook; i++, hook = t->tests[i]) {
		t->setup(t);
		printf("Executing test %i\n", i+1);
		hook();
		printf("Finished test %i\n", i+1);
		t->teardown(t);
	}
}

static void usage(const char *prog)
{
	printf("Usage: %s -t <test>\n", prog);
}

static void help(const char *prog)
{
	usage(prog);
	printf("Run a XFireDB unit test.\n" \
		"\n" \
		"All arguments are mandatory.\n" \
		"   -t, --test <test>       Run the given test\n" \
		"   -h, --help              Display this help text\n" \
		"\n" \
		"Available tests are:\n" \
		"\n" \
		"  storage:dict:single\n" \
		"  storage:dict:concurrent\n" \
		"  storage:dict:iterator\n" \
		"  storage:dict:database\n" \
		"\n" \
		"  storage:bio\n" \
		"  storage:disk\n" \
		"\n" \
		"  storage:red-black:single\n" \
		"  storage:red-black:concurrent\n" \
		"  storage:red-black:hashmap\n" \
		"  storage:red-black:set\n" \
		"\n" \
		"  os:bg\n" \
		"\n" \
		"  core:quotearg\n" \
		"  core:xfiredb\n" \
		"  core:sleep\n"   \
		"  core:bitops\n");
}

static struct option long_opts[] = {
	{"test", required_argument, 0, 't'},
	{"help", no_argument, 0, 'h'},
};

int main(int argc, char **argv)
{
	int c, optidx = 0;
	char *torun = NULL;
	struct dict *tests;
	size_t tmp;
	union entry_data data;
	struct unit_test *utest;

	while(true) {
		c = getopt_long(argc, argv, "t:h", long_opts, &optidx);
		if(c == -1)
			break;

		switch(c) {
		case 'h':
			help(argv[0]);
			exit(EXIT_SUCCESS);
			break;

		case 't':
			torun = optarg;
			break;

		case '?':
		case ':':
		default:
			usage(argv[0]);
			exit(EXIT_FAILURE);
			break;
		}
	}

	if(!torun) {
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	tests = dict_alloc();
	test_setup(tests);

	if(dict_lookup(tests, torun, &data, &tmp) != -XFIRE_OK) {
		fprintf(stderr, "Test '%s' not found, aborting.\n", torun);
		exit(EXIT_FAILURE);
	}

	utest = data.ptr;
	printf("Starting test '%s'\n", utest->name);
	test_exec(utest);

	dict_clear(tests);
	dict_free(tests);
	return -EXIT_SUCCESS;
}

