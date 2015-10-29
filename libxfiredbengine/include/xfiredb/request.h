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

#include <xfiredb/xfiredb.h>
#include <xfiredb/types.h>

typedef enum {
	RQ_LIST_RPUSH,
	RQ_LIST_LPUSH,
	RQ_LIST_REMOVE,
	RQ_LIST_LOOKUP,
	RQ_STRING_INSERT,
	RQ_STRING_REMOVE,
	RQ_STRING_LOOKUP,
} rq_type_t;

struct request {
	char *key;
	rq_type_t type;
	time_t stamp;

	struct rq_range {
		int start,
		    end;
		int *indexes;
	};
};

CDECL
CDECL_END

#endif
