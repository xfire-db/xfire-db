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

#include <xfire/xfire.h>
#include <xfire/types.h>
#include <xfire/bg.h>
#include <xfire/mem.h>
#include <xfire/error.h>

static void job1_handler(void *arg)
{
	printf("Job 1 handler fired\n");
}

static void job2_handler(void *arg)
{
	printf("Job 2 handler fired\n");
}

static void job3_handler(void *arg)
{
	printf("Job 3 handler fired\n");
}

int main(int argc, char **argv)
{
	bg_processes_init();
	bg_process_create("job1", &job1_handler, NULL);
	bg_process_create("job3", &job3_handler, NULL);
	bg_process_create("job2", &job2_handler, NULL);

	sleep(2);

	bg_process_signal("job1");
	bg_process_signal("job3");
	bg_process_signal("job2");
	sleep(2);

	bg_processes_exit();
	return -EXIT_SUCCESS;
}

