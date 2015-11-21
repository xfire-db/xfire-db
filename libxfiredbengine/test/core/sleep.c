/*
 *  XFireDB sleep unit test
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
#include <assert.h>
#include <unistd.h>
#include <time.h>
#include <unittest.h>

#include <xfiredb/xfiredb.h>
#include <xfiredb/time.h>

static void setup(struct unit_test *t)
{
}

static void teardown(struct unit_test *t)
{
}

static void sleep_test(void)
{
	long ns = 1.5 * 1000000000;
	time_t start;

	start = time(NULL);
	xfiredb_sleep_ns(ns);
	printf("time: %.2f\n", (double)(time(NULL) - start));
}

static test_func_t test_func_array[] = {sleep_test, NULL};
struct unit_test core_sleep_test = {
	.name = "core:sleep",
	.setup = setup,
	.teardown = teardown,
	.tests = test_func_array,
};

