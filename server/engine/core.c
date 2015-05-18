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
#include <xfire/list.h>
#include <xfire/container.h>

static struct request_pool **processors = NULL;
static int proc_num;

static xfire_spinlock_t db_lock;
static struct database **databases = NULL;
static int db_num;

static bool eng_compare_db_node(struct rb_node *node, const void *key)
{
	struct container *c;

	c = container_of(node, struct container, node);
	return !strcmp(c->key, key);
}

struct database *eng_init_db(struct database *db, const char *name)
{
	int len;

	rb_init_root(&db->root);
	db->root.cmp = &eng_compare_db_node;
	len = strlen(name);

	db->name = xfire_zalloc(len + 1);
	memcpy(db->name, name, len);

	return db;
}

static struct database *eng_alloc_db(const char *name)
{
	struct database *db = xfire_zalloc(sizeof(*db));

	return eng_init_db(db, name);
}

static void eng_free_db(struct database *db)
{
	if(!db)
		return;

	xfire_free(db->name);
	xfire_free(db);
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

void eng_create_db(const char *name)
{
	struct database *db = eng_alloc_db(name);
	eng_add_db(db);
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

struct rq_buff *rq_buff_alloc(struct request *parent)
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

void rq_buff_free(struct rq_buff *buffer)
{
	struct rq_buff *iterator;

	while(buffer) {
		iterator = buffer->next;
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

static void rq_buff_inflate(struct rq_buff *buff, size_t bytes)
{
	buff->data = xfire_zalloc(bytes);
}

static void eng_handle_request(struct request *rq, struct rq_buff *data)
{
	struct database *db;
	struct rb_node *node;
	struct list_head *lh;
	struct container *c;
	struct string *string;

	db = eng_get_db(rq->db_name);
	node = rb_get_node(&db->root, rq->hash, rq->key);
	
	switch(rq->type) {
	case RQ_LIST_INSERT:
		break;

	case RQ_LIST_REMOVE:
		if(!node)
			break;

		lh = node_get_data(node, LH_MAGIC);
		break;

	case RQ_LIST_LOOKUP:
		if(!node)
			break;

		lh = node_get_data(node, LH_MAGIC);
		break;

	case RQ_STRING_INSERT:
		c = eng_create_string_container(data);
		rb_set_key(&c->node, rq->hash);
		rb_insert(&db->root, &c->node, true);
		break;

	case RQ_STRING_REMOVE:
		if(!node)
			break;

		rb_remove(&db->root, rq->hash, rq->key);
		break;

	case RQ_STRING_LOOKUP:
		if(!node)
			break;

		string = node_get_data(node, S_MAGIC);
		if(!string) {
			rb_put_node(node);
			break;
		}

		data->length = string_length(string);
		rq_buff_inflate(data, data->length + 1);
		string_get(string, data->data, data->length + 1);
		break;

	default:
		break;
	}

	rb_put_node(node);
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

	for(; reply; reply = reply->next) {
		err = write(rq->fd, reply->data, reply->length);
		if(err)
			break;
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

	while(reply) {
		iterator = reply->next;
		
		xfire_free(reply);
		reply = iterator;
	}
}

static void eng_correct_request_range(struct request *request)
{
	struct rq_range *rng = &request->range;
	struct database *db;
	struct rb_node *node;
	struct list_head *lh;
	int end;

	end = rng->end;
	if(end < 0) {
		end += 1;
	
		/* Get the true ending of the range */
		db = eng_get_db(request->db_name);
		node = rb_find(&db->root, request->hash);
		lh = node_get_data(node, LH_MAGIC);
		if(!lh)
			return;

		rng->end = atomic_get(&lh->num);
		rng->end -= end;
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
	while(pool->head == NULL && !test_bit(RQP_EXIT_FLAG, &pool->flags))
		xfire_cond_wait(&pool->condi, &pool->lock);

	if(test_bit(RQP_EXIT_FLAG, &pool->flags))
		return NULL;
	next = eng_get_next_request(pool);
	xfire_mutex_unlock(&pool->lock);

	time(&tstamp);
	xfire_hash(next->key, &hash);
	next->stamp = tstamp;
	next->hash = hash;
	eng_correct_request_range(next);

	if(next->type == RQ_LIST_LOOKUP || 
			next->type == RQ_STRING_LOOKUP) {
		data = rq_buff_alloc(next);
		next->data = data;

		if(test_bit(RQ_MULTI_FLAG, &next->flags)) {
			range = next->range.end - next->range.start;
			multi = rq_buff_alloc_multi(next, range);
			multi->prev = data;
			data->next = multi;
			eng_handle_multi_request(next);
		} else {
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

		set_bit(RQP_EXIT_FLAG, &pool->flags);
		xfire_cond_signal(&pool->condi);
		while(!test_bit(RQP_STOPPED_FLAG, &pool->flags));
		xfire_thread_join(pool->proc);
		xfire_thread_destroy(pool->proc);
		xfire_cond_destroy(&pool->condi);
		xfire_mutex_destroy(&pool->lock);
		atomic_flags_destroy(&pool->flags);
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

#ifdef HAVE_DEBUG
#define DEBUG_DB_NAME "dbg-db"

void eng_create_debug_db(void)
{
	eng_create_db(DEBUG_DB_NAME);
}

void dbg_push_request(struct request *rq)
{
	struct request_pool *pool;

	pool = processors[0];
	eng_push_request(pool, rq);
}
#endif

