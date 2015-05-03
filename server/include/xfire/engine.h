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
} RQ_BUFF;

CDECL
extern void eng_init(int num);
extern void eng_exit(void);
extern void eng_push_request(struct request_pool *, struct request *);
CDECL_END

#endif

