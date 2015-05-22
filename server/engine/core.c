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
#include <xfire/list.h>
#include <xfire/container.h>

static struct request_pool **processors = NULL;
static int proc_num;

static xfire_spinlock_t db_lock;
static struct database **databases = NULL;
static int db_num;

struct database *eng_init_db(struct database *db, const char *name)
{
	int len;

	len = strlen(name);
	db->name = xfire_zalloc(len + 1);
	memcpy(db->name, name, len);

	return db;
}

static void __eng_free_db(struct database *db)
{
	int idx;

	if(!db)
		return;

	xfire_spin_lock(&db_lock);
	for(idx = 0; idx < db_num; idx++) {
		if(databases[idx] == db) {
			databases[idx] = NULL;
			break;
		}
	}
	xfire_spin_unlock(&db_lock);
	xfire_free(db->name);

	db->free(db);
}

void eng_free_db(const char *name)
{
	struct database *db;

	db = eng_get_db(name);
	if(!db)
		return;

	__eng_free_db(db);
}

void eng_add_db(struct database *db)
{
	int idx;

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

struct database *eng_get_db(const char *name)
{
	int idx;
	struct database *db = NULL;

	xfire_spin_lock(&db_lock);
	for(idx = 0; idx < db_num; idx++) {
		db = databases[idx];
		if(!db)
			continue;

		if(!strcmp(db->name, name)) {
			break;
		} else if((idx+1) >= db_num) {
			db = NULL;
			break;
		}
	}
	xfire_spin_unlock(&db_lock);

	return db;
}

static struct rq_buff *__rq_buff_alloc(int index)
{
	struct rq_buff *data;

	data = xfire_zalloc(sizeof(*data));
	atomic_flags_init(&data->flags);
	data->index = index;

	return data;
}

struct rq_buff *rq_buff_alloc(struct request *parent)
{
	struct rq_buff *data;

	if(parent->domain.indexes)
		data = __rq_buff_alloc(parent->domain.indexes[0]);
	else
		data = __rq_buff_alloc(-1);

	data->parent = parent;
	return data;
}

static struct rq_buff *rq_buff_alloc_multi(struct request *parent, int num)
{
	struct rq_buff *data,
		       *iterator,
		       *head;
	int idx = 0;

	if(test_bit(RQ_HAS_RANGE_FLAG, &parent->flags))
		num = parent->domain.range.end - parent->domain.range.start;


	if(!parent || !num)
		return NULL;

	if(!test_bit(RQ_HAS_RANGE_FLAG, &parent->flags))
		head = __rq_buff_alloc(parent->domain.indexes[idx]);
	else
		head = __rq_buff_alloc(-1);
	iterator = head;
	head->parent = parent;
	idx++;

	for(; idx < num; idx++) {
		if(!test_bit(RQ_HAS_RANGE_FLAG, &parent->flags))
			data = __rq_buff_alloc(parent->domain.indexes[idx]);
		else
			data = __rq_buff_alloc(-1);
		data->parent = parent;

		data->prev = iterator;
		data->next = NULL;

		iterator->next = data;
		iterator = data;
	}

	return head;
}

static void rq_buff_inflate(struct rq_buff *buff, size_t bytes)
{
	buff->data = xfire_zalloc(bytes);
	set_bit(RQB_INFLATED_FLAG, &buff->flags);
}

static void rq_buff_deflate(struct rq_buff *buff)
{
	if(buff->data && test_bit(RQB_INFLATED_FLAG, &buff->flags))
		xfire_free(buff->data);
}

void rq_buff_free(struct rq_buff *buffer)
{
	struct rq_buff *iterator;

	while(buffer) {
		iterator = buffer->next;
		rq_buff_deflate(buffer);
		xfire_free(buffer);
		buffer = iterator;
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
	if(pool->head == pool->tail) {
		pool->head = NULL;
		pool->tail = NULL;
	} else {
		pool->head = pool->head->next;
	}

	return rv;
}

static struct container *eng_create_string_container(struct rq_buff *data)
{
	struct container *c;
	const char *key = data->parent->key;

	c = xfire_zalloc(sizeof(*c));
	container_init(c, key, S_MAGIC);
	container_set_string(c, data->data);

	return c;
}

static struct container *eng_create_list_container(struct rq_buff *data)
{
	struct container *c;
	const char *key = data->parent->key;

	c = xfire_zalloc(sizeof(*c));
	container_init(c, key, LH_MAGIC);

	return c;
}

static void eng_destroy_container(struct container *c)
{
	container_destroy(c, c->magic);
	xfire_free(c);
}

static void eng_push_list(struct rq_buff *data, struct list_head *lh,
				rq_type_t type)
{
	struct string *s = xfire_zalloc(sizeof(*s));

	string_init(s);
	string_set(s, data->data);

	if(type == RQ_LIST_RPUSH)
		list_rpush(lh, &s->entry);
	else
		list_lpush(lh, &s->entry);
}

static struct list *eng_get_list_index(struct list_head *head, int index)
{
	int idx, len;
	struct list *c;

	len = list_length(head);
	if(len <= index)
		return NULL;

	idx = 0;
	list_for_each(head, c) {
		if(idx == index)
			return c;

		idx++;
	}

	return NULL;
}

static void raw_eng_handle_request(struct request *rq, struct database *db,
					struct container *c,
					struct rq_buff *data)
{
	struct list_head *lh;
	struct string *string;
	struct list *list;

	switch(rq->type) {
	case RQ_LIST_RPUSH:
	case RQ_LIST_LPUSH:
		if(!c) {
			c = eng_create_list_container(data);
			db->insert(db, rq->hash, c);
		}

		lh = container_get_data(c, LH_MAGIC);
		eng_push_list(data, lh, rq->type);
		break;

	case RQ_LIST_REMOVE:
		if(!c)
			break;

		lh = container_get_data(c, LH_MAGIC);
		break;

	case RQ_LIST_LOOKUP:
		if(!c)
			break;

		lh = container_get_data(c, LH_MAGIC);
		list = eng_get_list_index(lh, data->index);
		if(!list)
			return;

		string = container_of(list, struct string, entry);
		data->length = string_length(string);
		rq_buff_inflate(data, data->length);
		string_get(string, data->data, data->length);
		break;

	case RQ_STRING_INSERT:
		c = eng_create_string_container(data);
		db->insert(db, rq->hash, c);
		break;

	case RQ_STRING_REMOVE:
		if(!c)
			break;

		c = db->remove(db, rq->hash, rq->key);
		if(!c)
			break;

		string = container_get_data(c, S_MAGIC);
		if(!string)
			break;

		if(c) {
			data->length = string_length(string);
			rq_buff_inflate(data, data->length);
			string_get(string, data->data, data->length);
			eng_destroy_container(c);
		}

		break;

	case RQ_STRING_LOOKUP:
		if(!c)
			break;

		string = container_get_data(c, S_MAGIC);
		if(!string)
			break;

		data->length = string_length(string);
		rq_buff_inflate(data, data->length);
		string_get(string, data->data, data->length);
		break;

	default:
		break;
	}
}


static void eng_handle_range_request(struct request *rq, struct database *db,
					struct container *c, struct rq_buff *d)
{
	struct list_head *lh;
	struct string *string;
	struct list *list;
	struct request_domain *dom = &rq->domain;
	int idx = 0;

	switch(rq->type) {
	case RQ_LIST_REMOVE:
		if(!c)
			break;

		lh = container_get_data(c, LH_MAGIC);
		break;

	case RQ_LIST_LOOKUP:
		if(!c)
			break;

		lh = container_get_data(c, LH_MAGIC);
		if(list_length(lh) <= dom->range.start)
			break;

		list_for_each(lh, list) {
			if(idx < dom->range.start)
				continue;
			else if(!d)
				break;
			

			string = container_of(list, struct string, entry);
			d->length = string_length(string);
			rq_buff_inflate(d, d->length);
			string_get(string, d->data, d->length);

			d = d->next;
			idx++;
		}
		break;

	default:
		break;
	}
}

static void eng_handle_request(struct request *rq, struct rq_buff *data)
{
	struct database *db;
	struct container *c;

	db = eng_get_db(rq->db_name);
	c = db->lookup(db, rq->hash, rq->key);

	if(test_bit(RQ_HAS_RANGE_FLAG, &rq->flags)) {
		eng_handle_range_request(rq, db, c, data);
		return;
	}

	raw_eng_handle_request(rq, db, c, data);
}

static inline void eng_handle_multi_request(struct request *rq)
{
	struct request_domain *dom = &rq->domain;
	struct rq_buff *buffer;
	int i;

	buffer = rq->data;

	if(test_bit(RQ_HAS_RANGE_FLAG, &rq->flags)) {
		eng_handle_request(rq, buffer);
		return;
	}

	for(i = 0; i < dom->num && buffer; i++) {
		eng_handle_request(rq, buffer);
		buffer = buffer->next;
	}
}

static int eng_reply(struct request *rq, struct reply *reply)
{
	int err;

	for(; reply; reply = reply->next)
		err = write(rq->fd, reply->data, reply->length);

	return err;
}

static void reply_add(struct reply *prev, struct reply *new)
{
	prev->next = new;
	new->prev = prev;
	new->next = NULL;
}

static struct reply *eng_build_reply(struct rq_buff *data)
{
	struct reply *head,
		     *reply,
		     *prev;
	struct rq_buff *iterator;
	u16 i = 1;

	head = xfire_zalloc(sizeof(*head));
	head->num = 0;
	head->length = data->length;
	head->data = data->data;
	prev = head;

	if(!data->next)
		return head;

	for(iterator = data->next; iterator; iterator = iterator->next) {
		reply = xfire_zalloc(sizeof(*head));
		reply->num = i;
		reply->length = iterator->length;
		reply->data = iterator->data;

		reply_add(prev, reply);
		prev = reply;
		i++;
	}

	return head;
}

static void eng_release_reply(struct reply *reply)
{
	struct reply *iterator;

	while(reply) {
		iterator = reply->next;
		
		xfire_free(reply);
		reply = iterator;
	}
}

static int eng_get_list_size(struct request *request)
{
	struct database *db;
	struct container *c;
	struct list_head *lh;

	db = eng_get_db(request->db_name);
	c = db->lookup(db, request->hash, request->key);
	if(!c)
		return -1;

	lh = container_get_data(c, LH_MAGIC);

	if(!lh)
		return -1;

	return atomic_get(&lh->num);
}

static void eng_correct_request_range(struct request *request)
{
	struct request_domain *dom = &request->domain;
	int end, start, size, i, index;

	start = dom->range.start;
	end = dom->range.end;
	size = eng_get_list_size(request);

	if(size < 0)
		return;

	if(!test_bit(RQ_HAS_RANGE_FLAG, &request->flags)) {
		size -= 1; /* list are 0 counted */

		for(i = 0; i < dom->num; i++) {
			index = dom->indexes[i];
			if(index < 0) {
				index += 1;
				dom->indexes[i] = size - index;
			}
		}

		return;
	}

	if(end < 0) {
		end += 1;
	
		dom->range.end = size;
		dom->range.end -= end;
		end = dom->range.end;
	}

	if(start == end) {
		rq_add_rng_index(request, &start, 1);
		clear_bit(RQ_HAS_RANGE_FLAG, &request->flags);
	}
}

static struct request *eng_processor(struct request_pool *pool)
{
	struct request *next;
	struct rq_buff *data;
	struct reply *reply;
	time_t tstamp;
	u64 hash;
	int range;

	xfire_mutex_lock(&pool->lock);
	while(pool->head == NULL && !test_bit(RQP_EXIT_FLAG, &pool->flags))
		xfire_cond_wait(&pool->condi, &pool->lock);

	if(test_bit(RQP_EXIT_FLAG, &pool->flags)) {
		xfire_mutex_unlock(&pool->lock);
		return NULL;
	}

	next = eng_get_next_request(pool);
	xfire_mutex_unlock(&pool->lock);

	time(&tstamp);
	xfire_hash(next->key, &hash);
	next->stamp = tstamp;
	next->hash = hash;
	eng_correct_request_range(next);

	if(next->type == RQ_LIST_LOOKUP || 
			next->type == RQ_STRING_LOOKUP ||
			next->type == RQ_LIST_REMOVE ||
			next->type == RQ_STRING_REMOVE) {

		if(test_bit(RQ_MULTI_FLAG, &next->flags)) {
			range = next->domain.num;
			data = rq_buff_alloc_multi(next, range);
			next->data = data;
			eng_handle_multi_request(next);
		} else {
			data = rq_buff_alloc(next);
			next->data = data;
			eng_handle_request(next, next->data);
		}
	} else {
		if(test_bit(RQ_MULTI_FLAG, &next->flags))
			eng_handle_multi_request(next);
		else
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

		if(handle) {
			xfire_mutex_lock(&handle->lock);
			set_bit(RQ_PROCESSED_FLAG, &handle->flags);
			xfire_cond_signal(&handle->condi);
			xfire_mutex_unlock(&handle->lock);
		}

	} while(!test_bit(RQP_EXIT_FLAG, &pool->flags));

	set_bit(RQP_STOPPED_FLAG, &pool->flags);
	xfire_thread_exit(NULL);
}

void eng_push_request(struct request_pool *pool, struct request *request)
{
	struct request *tail;

	xfire_mutex_lock(&pool->lock);
	tail = pool->tail;

	pool->tail = request;

	if(tail) {
		tail->next = request;
		request->prev = tail;
	} else {
		pool->head = request;
	}

	xfire_cond_signal(&pool->condi);
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

		xfire_mutex_lock(&pool->lock);
		set_bit(RQP_EXIT_FLAG, &pool->flags);
		xfire_cond_signal(&pool->condi);
		xfire_mutex_unlock(&pool->lock);

		xfire_thread_join(pool->proc);
		xfire_thread_destroy(pool->proc);

		xfire_cond_destroy(&pool->condi);
		xfire_mutex_destroy(&pool->lock);
		atomic_flags_destroy(&pool->flags);
		xfire_free(pool);
	}

	for(i = 0; i < db_num; i++)
		__eng_free_db(databases[i]);

	xfire_spinlock_destroy(&db_lock);
	xfire_free(databases);
	xfire_free(processors);
}

#ifdef HAVE_DEBUG
#define DEBUG_DB_NAME "dbg-db"

void dbg_push_request(struct request *rq)
{
	struct request_pool *pool;

	pool = processors[0];
	eng_push_request(pool, rq);
}
#endif

