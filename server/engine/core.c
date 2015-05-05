/*
 *  ENGINE CORE
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
#include <time.h>

#include <xfire/xfire.h>
#include <xfire/types.h>
#include <xfire/flags.h>
#include <xfire/request.h>
#include <xfire/engine.h>
#include <xfire/hash.h>
#include <xfire/os.h>
#include <xfire/mem.h>
#include <xfire/rbtree.h>

static struct request_pool **processors = NULL;
static int proc_num;

struct database {
	char *name;
	struct rb_root root;
};

static xfire_spinlock_t db_lock;
static struct database **databases = NULL;
static int db_num;

static struct database *eng_alloc_db(const char *name)
{
	int len;
	struct database *db = xfire_zalloc(sizeof(*db));

	rb_init_root(&db->root);
	len = strlen(name);

	db->name = xfire_zalloc(len + 1);
	memcpy(db->name, name, len);

	return db;
}

static void eng_free_db(struct database *db)
{
	if(!db)
		return;

	xfire_free(db->name);
	xfire_free(db);
}

void eng_create_db(const char *name)
{
	int idx;
	struct database *db = eng_alloc_db(name);

	xfire_spin_lock(&db_lock);
	for(idx = 0; idx < db_num; idx++) {
		if(databases[idx]) {
			continue;
		} else {
			databases[idx] = db;
			xfire_spin_unlock(&db_lock);
			return;
		}
	}

	db_num += 5;
	databases = realloc(databases, sizeof(void*) * db_num);
	databases[idx] = db;
	xfire_spin_unlock(&db_lock);
}

static struct rq_buff *rq_buff_alloc(struct request *parent)
{
	struct rq_buff *data;

	data = xfire_zalloc(sizeof(*data));
	data->parent = parent;
	atomic_flags_init(&data->flags);

	return data;
}

static struct rq_buff *rq_buff_alloc_multi(struct request *parent, int num)
{
	struct rq_buff *data,
		       *iterator,
		       *head;
	int idx = 0;

	if(!parent || !num)
		return NULL;

	head = rq_buff_alloc(parent);
	iterator = head;
	idx++;

	for(; idx < num; idx++) {
		data = rq_buff_alloc(parent);

		data->prev = iterator;
		data->next = NULL;

		iterator->next = data;
		iterator = data;
	}

	return head;
}

static void rq_buff_free(struct rq_buff *buffer)
{
	struct rq_buff *iterator;

	for(iterator = buffer->next; buffer; buffer = iterator,
						iterator = iterator->next) {
		free(buffer);
	}
}

static struct request_pool *rq_pool_alloc(void)
{
	struct request_pool *pool;

	pool = xfire_zalloc(sizeof(*pool));
	xfire_cond_init(&pool->condi);
	xfire_mutex_init(&pool->lock);
	atomic_flags_init(&pool->flags);

	return pool;
}

static struct request *eng_get_next_request(struct request_pool *pool)
{
	struct request *rv;

	rv = pool->head;

	/* set the pool head to the next request */
	pool->head = pool->head->next;
	return rv;
}

static void eng_handle_request(struct request *rq, struct rq_buff *data)
{
}

static inline void eng_handle_multi_request(struct request *rq)
{
	struct rq_buff *buffer;
	int len, i;

	len = rq->range.end - rq->range.start;
	len += 1; /* also include the base entry */
	buffer = rq->data;

	for(i = 0; i < len && buffer; i++) {
		eng_handle_request(rq, buffer);
		buffer = buffer->next;
	}
}

static int eng_reply(struct request *rq, struct reply *reply)
{
	int err;
	size_t length = sizeof(reply->num) + sizeof(reply->length);

	for(; reply; reply = reply->next) {
		err = write(rq->fd, &reply->num, length);
		if(!err) {
			err = write(rq->fd, reply->data, reply->length);
			if(err)
				break;
		} else {
			break;
		}
	}

	return err;
}

static struct reply *eng_build_reply(struct rq_buff *data)
{
	struct reply *head,
		     *reply,
		     *prev;
	struct rq_buff *iterator;
	u16 i = 0;

	head = xfire_zalloc(sizeof(*head));
	head->num = 0;
	head->length = data->length;
	head->data = data->data;
	prev = head;

	for(iterator = data->next; iterator; iterator = iterator->next, i++) {
		reply = xfire_zalloc(sizeof(*reply));
		reply->num = i;
		reply->length = iterator->length;
		reply->data = iterator->data;

		prev->next = reply;
		reply->prev = prev;
	}

	return head;
}

static void eng_release_reply(struct reply *reply)
{
	struct reply *iterator;

	for(iterator = reply->next; reply; iterator = iterator->next,
						reply = iterator) {
		xfire_free(reply);
	}
}

static struct request *eng_processor(struct request_pool *pool)
{
	struct request *next;
	struct rq_buff *data,
		       *multi;
	struct reply *reply;
	time_t tstamp;
	u64 hash;
	int range;

	xfire_mutex_lock(&pool->lock);
	while(pool->head == NULL)
		xfire_cond_wait(&pool->condi, &pool->lock);

	next = eng_get_next_request(pool);
	xfire_mutex_unlock(&pool->lock);

	data = rq_buff_alloc(next);
	time(&tstamp);
	xfire_hash(next->key, &hash);

	next->data = data;
	next->stamp = tstamp;
	next->hash = hash;

	if(test_bit(RQ_MULTI_FLAG, &next->flags)) {
		range = next->range.end - next->range.start;
		multi = rq_buff_alloc_multi(next, range);
		multi->prev = data;
		data->next = multi;
		eng_handle_multi_request(next);
	} else {
		eng_handle_request(next, next->data);
	}

	reply = eng_build_reply(next->data);
	eng_reply(next, reply);
	eng_release_reply(reply);
	rq_buff_free(next->data);
	return next;
}

void *eng_processor_thread(void *arg)
{
	struct request_pool *pool = arg;
	struct request *handle;

	do {
		handle = eng_processor(pool);
		xfire_mutex_lock(&handle->lock);
		xfire_cond_signal(&handle->condi);
		xfire_mutex_unlock(&handle->lock);
	} while(!test_bit(RQP_EXIT_FLAG, &pool->flags));

	xfire_thread_exit(NULL);
}

void eng_push_request(struct request_pool *pool, struct request *request)
{
	struct request *tail;

	xfire_mutex_lock(&pool->lock);
	tail = pool->tail;
	if(tail)
		tail->next = request;
	else
		pool->head = request;

	request->prev = tail;
	pool->tail = request;
	xfire_mutex_unlock(&pool->lock);
}

#define POOL_NAME_LENGTH 12
#define XFIRE_DB_INIT_NUM 5

void eng_init(int num)
{
	int i;
	struct request_pool *pool;

	processors = xfire_calloc(num, sizeof(void*));
	databases = xfire_calloc(XFIRE_DB_INIT_NUM, sizeof(void*));
	db_num = XFIRE_DB_INIT_NUM;
	proc_num = num;
	xfire_spinlock_init(&db_lock);

	for(i = 0; i < num; i++) {
		pool = rq_pool_alloc();
		snprintf((char*)pool->name, POOL_NAME_LENGTH, "proc %d", i);
		pool->proc = xfire_create_thread(pool->name,
						 &eng_processor_thread,
						 pool);
		processors[i] = pool;
	}

	/* initialise the sentinel */
}

void eng_exit(void)
{
	struct request_pool *pool;
	int i = 0;

	for(; i < proc_num; i++) {
		pool = processors[i];

		set_bit(RQP_EXIT_FLAG, &pool->flags);
		xfire_thread_cancel(pool->proc);
		xfire_thread_destroy(pool->proc);
		xfire_cond_destroy(&pool->condi);
		xfire_mutex_destroy(&pool->lock);
		xfire_free(pool);
	}

	xfire_spin_lock(&db_lock);
	for(i = 0; i < db_num; i++)
		eng_free_db(databases[i]);
	xfire_spin_unlock(&db_lock);

	xfire_spinlock_destroy(&db_lock);
	xfire_free(databases);
	xfire_free(processors);
}

