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

static struct request *eng_get_next_request(struct request_pool *pool)
{
	return NULL;
}

static struct request *eng_handle_request(struct request *rq)
{
	return NULL;
}

static void eng_processor(struct request_pool *pool)
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

	if(test_and_swap_bit(RQ_MULTI_FLAG, &next->flags, &data->flags)) {
		range = next->range.end - next->range.start;
		multi = rq_buff_alloc_multi(next, range - 1);
		multi->prev = data;
		data->next = multi;

	} else {
	}

	next->data = data;
	next->stamp = tstamp;
	next->hash = hash;

	eng_handle_request(next);
}

