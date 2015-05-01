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

#include <xfire/request.h>
#include <xfire/xfire.h>
#include <xfire/types.h>
#include <xfire/flags.h>
#include <xfire/os.h>

struct rq_buff *rq_buff_alloc(struct request *parent)
{
	struct rq_buff *data;

	data = malloc(sizeof(*data));
	memset(data, 0x0, sizeof(*data));

	data->parent = parent;
	atomic_flags_init(&data->flags);

	return data;
}

struct rq_buff *rq_buff_alloc_multi(struct request *parent, int num)
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

