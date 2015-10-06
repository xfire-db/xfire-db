/*
 *  ENGINE header
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

#ifndef __ENGINE__H__
#define __ENGINE__H__

#include <xfire/xfire.h>
#include <xfire/types.h>
#include <xfire/request.h>

typedef struct reply {
	struct reply *next,
		     *prev;
	u16 num;
	u32 length;
	void *data;
} REPLY;

typedef struct rq_buff {
	struct request *parent;
	struct rq_buff *next,
		       *prev;

	atomic_flags_t flags;

	void *data;
	u32 length;

	int index;
} RQ_BUFF;

#define RQB_INFLATED_FLAG 0

#include <xfire/rbtree.h>
typedef struct db {
	char *name;

	void *(*lookup)(struct db *db, u64 key, void *arg);
	bool (*insert)(struct db *db, u64 key, void *data);
	void *(*remove)(struct db *db, u64 key, void *arg);
	void (*free)(struct db *db);
} DATABASE;

CDECL
extern void eng_init(int num);
extern void eng_exit(void);
extern void eng_push_request(struct request_pool *, struct request *);
extern struct db *eng_get_db(const char *name);
extern struct rq_buff *rq_buff_alloc(struct request *parent);
extern void rq_buff_free(struct rq_buff *buffer);
extern void eng_create_debug_db(void);
extern void dbg_push_request(struct request *rq);
extern struct db *eng_init_db(struct db *db, const char *name);
extern void eng_add_db(struct db *db);
CDECL_END

#endif

