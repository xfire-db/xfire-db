/*
 *  XFIRE HASHING
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

#include <xfire/xfire.h>
#include <xfire/types.h>
#include <xfire/engine.h>
#include <xfire/request.h>
#include <xfire/mem.h>

static const char *request_key_array[] = {
	"user:bietje:cache:GC54JKP:route",
	"user:bietje:cache:GCJ89KP:route",
	"user:bietje:cache:GC8YIND:route",
	"user:bietje:cache:GCLIP21:route",
	"user:bietje:cache:GC4VAG5:route",
	"user:bietje:cache:GC30FQ4:route",
};

static const int request_range_array[] = {
	3, 4,
	5, 5,
	0, 20,
	23, 56,
	8, 8,
	0, 100,
};

#define TEST_RQ_LENGTH 6

static struct request **request_array;

static void test_build_requests(void)
{
	int i;

	request_array = xfire_calloc(TEST_RQ_LENGTH, sizeof(void*));

	for(i = 0; i < TEST_RQ_LENGTH; i++) {
		request_array[i] = rq_alloc(request_key_array[i],
						request_range_array[i],
						request_range_array[i+1]);
		request_array[i]->fd = fileno(stdout);
	}
}

static void test_cleanup_requests(void)
{
	int i;

	for(i = 0; i < TEST_RQ_LENGTH; i++)
		rq_free(request_array[i]);

	xfire_free(request_array);
}

int main(int argc, char **argv)
{
	eng_init(8);
	test_build_requests();
	test_cleanup_requests();
	eng_exit();
	return -EXIT_SUCCESS;
}

