/*
 *  Background processes
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

#include <xfiredb/xfiredb.h>
#include <xfiredb/types.h>
#include <xfiredb/bg.h>
#include <xfiredb/mem.h>
#include <xfiredb/error.h>

static bool j1_trigger, j2_trigger, j3_trigger;

static void job1_handler(void *arg)
{
	j1_trigger = true;
}

static void job2_handler(void *arg)
{
	j2_trigger = true;
}

static void job3_handler(void *arg)
{
	j3_trigger = true;
}

static void setup(struct unit_test *t)
{
	j1_trigger = j2_trigger = j3_trigger = false;
	bg_processes_init();
}

static void teardown(struct unit_test *t)
{
	bg_processes_exit();
	assert(j1_trigger);
	assert(j2_trigger);
	assert(j3_trigger);
}

static void test_bg(void)
{
	bg_process_create("job1", &job1_handler, NULL);
	bg_process_create("job3", &job3_handler, NULL);
	bg_process_create("job2", &job2_handler, NULL);

	bg_process_signal("job1");
	bg_process_signal("job3");
	bg_process_signal("job2");
}

static test_func_t test_func_array[] = {test_bg, NULL};
struct unit_test bg_test = {
	.name = "os:bg",
	.setup = setup,
	.teardown = teardown,
	.tests = test_func_array,
};

