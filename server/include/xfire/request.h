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

#ifndef __REQUEST__H__
#define __REQUEST__H__

#include <time.h>

#include <xfire/xfire.h>
#include <xfire/types.h>
#include <xfire/flags.h>
#include <xfire/os.h>

struct rq_buff;

typedef enum {
	RQ_INSERT,
	RQ_REMOVE,
	RQ_LOOKUP,
} rq_type_t;

typedef struct request {
	int fd;

	struct request *next,
		       *prev;

	rq_type_t type;
	const char *key;
	u64 hash;
	struct rq_buff *data;

	time_t stamp;
	xfire_mutex_t lock;
	xfire_cond_t condi;

	struct rq_range {
		int start,
		    end;
	} range;
	atomic_flags_t flags;
} REQUEST;

typedef struct request_pool {
	struct request *head,
		       *tail,
		       *curr;

	const char name[13];
	struct thread *proc;

	atomic_flags_t flags;
	xfire_mutex_t lock;
	xfire_cond_t condi;
} REQUEST_POOL;

#define RQP_EXIT_FLAG		0

#define RQ_INSERT_FLAG		0
#define RQ_REMOVE_FLAG		1
#define RQ_LOOKUP_FLAG		2
#define RQ_FAULT_FLAG		3
#define RQ_PROCESSING_FLAG	4
#define RQ_MULTI_FLAG		5

CDECL
void *eng_processor_thread(void *arg);
CDECL_END

#endif
