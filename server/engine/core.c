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
#include <time.h>

#include <xfire/xfire.h>
#include <xfire/types.h>
#include <xfire/flags.h>
#include <xfire/request.h>
#include <xfire/engine.h>
#include <xfire/hash.h>
#include <xfire/os.h>
#include <xfire/mem.h>

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

static struct request_pool *rq_pool_alloc(void)
{
	struct request_pool *pool;

	pool = xfire_zalloc(sizeof(*pool));
	xfire_cond_init(&pool->condi);
	xfire_mutex_init(&pool->lock);

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

static void eng_reply(struct request *rq)
{
}

static struct request *eng_processor(struct request_pool *pool)
{
	struct request *next;
	struct rq_buff *data,
		       *multi;
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

	if(test_and_swap_bit(RQ_MULTI_FLAG, &next->flags, &data->flags)) {
		range = next->range.end - next->range.start;
		multi = rq_buff_alloc_multi(next, range);
		multi->prev = data;
		data->next = multi;
		eng_handle_multi_request(next);
	} else {
		eng_handle_request(next, next->data);
	}

	eng_reply(next);
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
	} while(true);

	xfire_thread_exit(NULL);
}

