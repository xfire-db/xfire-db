/*
 *  REQUEST header
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
#include <string.h>

#include <xfire/xfire.h>
#include <xfire/request.h>
#include <xfire/types.h>
#include <xfire/flags.h>
#include <xfire/mem.h>
#include <xfire/os.h>

struct request *rq_alloc(const char *db, const char *key, int start, int end)
{
	struct request *request;
	int len;

	len = strlen(key);
	request = xfire_zalloc(sizeof(*request));
	request->key = xfire_zalloc(len + 1);

	memcpy(request->key, key, len);
	xfire_mutex_init(&request->lock);
	xfire_cond_init(&request->condi);
	atomic_flags_init(&request->flags);

	len = strlen(db);
	request->db_name = xfire_zalloc(len + 1);
	memcpy(request->db_name, db, len);

	request->range.start = start;
	request->range.end = end;

	if(start != end)
		set_bit(RQ_MULTI_FLAG, &request->flags);

	return request;
}

void rq_free(struct request *rq)
{
	if(!rq)
		return;

	xfire_free(rq->key);
	xfire_free(rq->db_name);
	xfire_mutex_destroy(&rq->lock);
	xfire_cond_destroy(&rq->condi);
	atomic_flags_destroy(&rq->flags);
	xfire_free(rq);
}

