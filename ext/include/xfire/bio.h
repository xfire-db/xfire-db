/*
 *  Background I/O header
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

#ifndef __BIO_H__
#define __BIO_H__

#include <xfire/xfire.h>
#include <xfire/types.h>
#include <xfire/dict.h>

struct bio_q;

struct bio_q_head {
	struct bio_q *next,
		     *tail;
	struct job *job;
};

struct bio_q {
	struct bio_q *next,
		     *prev;
};

CDECL
extern void bio_init(void);
extern int bio_update(struct dict_entry *e);
extern void bio_exit(void);
CDECL_END

#endif

