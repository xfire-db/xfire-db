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
#include <fcntl.h>

#include <xfire/xfire.h>
#include <xfire/types.h>
#include <xfire/engine.h>
#include <xfire/request.h>
#include <xfire/mem.h>
#include <xfire/os.h>
#include <xfire/rb_db.h>

#if 0
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
#define TEST_DB_NAME "test_db"

static struct request **request_array;

static void test_build_requests(void)
{
	int i;

	request_array = xfire_calloc(TEST_RQ_LENGTH, sizeof(void*));

	for(i = 0; i < TEST_RQ_LENGTH; i++) {
		request_array[i] = rq_alloc(TEST_DB_NAME,
						request_key_array[i],
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
#endif

#define TEST_STRING_ENTRY0 "abc00\n"
#define TEST_STRING_ENTRY1 "bietje\n"

static inline void test_rq_wait(struct request *rq)
{
	xfire_mutex_lock(&rq->lock);
	while(!test_bit(RQ_PROCESSED_FLAG, &rq->flags))
		xfire_cond_wait(&rq->condi, &rq->lock);
	xfire_mutex_unlock(&rq->lock);
}

static void test_string_insert(void)
{
	struct request *a, *b;

	a = rq_alloc(DEBUG_DB_NAME, "user:bietje:password", 0, 0);
	b = rq_alloc(DEBUG_DB_NAME, "user:bietje:email", 0, 0);

	a->data = rq_buff_alloc(a);
	a->data->data = TEST_STRING_ENTRY0;
	a->data->length = sizeof(TEST_STRING_ENTRY0);
	a->type = RQ_STRING_INSERT;

	b->data = rq_buff_alloc(b);
	b->data->data = TEST_STRING_ENTRY1;
	b->data->length = sizeof(TEST_STRING_ENTRY1);
	b->type = RQ_STRING_INSERT;

	a->fd = b->fd = fileno(stdout);
	dbg_push_request(a);
	test_rq_wait(a);
	dbg_push_request(b);
	test_rq_wait(b);

	rq_free(a);
	rq_free(b);
}

static void test_string_lookup(void)
{
	struct request *a, *b;

	a = rq_alloc(DEBUG_DB_NAME, "user:bietje:password", 0, 0);
	b = rq_alloc(DEBUG_DB_NAME, "user:bietje:email", 0, 0);

	a->type = b->type = RQ_STRING_LOOKUP;
	a->fd = b->fd = fileno(stdout);

	dbg_push_request(a);
	test_rq_wait(a);
	dbg_push_request(b);
	test_rq_wait(b);

	rq_free(a);
	rq_free(b);
}

static void test_string_destroy(void)
{
	struct request *a, *b;

	a = rq_alloc(DEBUG_DB_NAME, "user:bietje:password", 0, 0);
	b = rq_alloc(DEBUG_DB_NAME, "user:bietje:email", 0, 0);

	a->type = b->type = RQ_STRING_REMOVE;
	a->fd = b->fd = fileno(stdout);

	dbg_push_request(a);
	test_rq_wait(a);
	dbg_push_request(b);
	test_rq_wait(b);

	rq_free(a);
	rq_free(b);
}

static void test_create_db(void)
{
	rbdb_alloc(DEBUG_DB_NAME);
}

int main(int argc, char **argv)
{
	eng_init(8);
	test_create_db();
	test_string_insert();
	test_string_lookup();
	test_string_destroy();

	eng_exit();
	return -EXIT_SUCCESS;
}

