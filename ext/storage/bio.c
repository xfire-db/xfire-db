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

#include <stdlib.h>
#include <stdio.h>

#include <xfire/xfire.h>
#include <xfire/types.h>
#include <xfire/bg.h>
#include <xfire/bio.h>
#include <xfire/database.h>
#include <xfire/mem.h>
#include <xfire/os.h>
#include <xfire/error.h>

static struct bio_q_head *bio_q;

static inline struct bio_q *bio_queue_pop(void)
{
	struct bio_q *tail;

	xfire_spin_lock(&bio_q->lock);
	tail = bio_q->tail;
	
	if(tail) {
		bio_q->tail = tail->prev;

		if(tail->prev) {
			tail->prev->next = NULL;
		} else {
			bio_q->next = NULL;
		}

		tail->next = tail->prev = NULL;
	}
	xfire_spin_unlock(&bio_q->lock);

	return tail;
}

static void bio_worker(void *arg)
{
}

void bio_init(struct database *db)
{
	bio_q = xfire_zalloc(sizeof(*bio_q));
	xfire_spinlock_init(&bio_q->lock);
	bio_q->job = bg_process_create("bio-worker", &bio_worker, db);
}

void bio_exit(void)
{
	bg_process_stop("bio-worker");
	xfire_spinlock_destroy(&bio_q->lock);
	xfire_free(bio_q);
}

void bio_queue_add(char *key, char *arg, char *newdata, bio_operation_t op)
{
	struct bio_q *q;

	q = xfire_zalloc(sizeof(*q));
	q->key = key;
	q->arg = arg;
	q->newdata = newdata;
	q->operation = op;

	xfire_spin_lock(&bio_q->lock);
	if(bio_q->next) {
		q->next = bio_q->next;
		bio_q->next->prev = q;
		bio_q->next = q;
	} else {
		/* first entry */
		bio_q->next = bio_q->tail = q;
	}
	xfire_spin_unlock(&bio_q->lock);
}

#ifdef HAVE_DEBUG
void dbg_bio_queue(void)
{
	struct bio_q *c;
	int i;

	/* allocate a bunch of queue's */
	bio_queue_add("test-first", NULL, NULL, STRING_DEL);
	for(i = 0; i < 5; i++)
		bio_queue_add("test-key", NULL, NULL, STRING_DEL);

	c = bio_queue_pop();
	printf("Bio queue pop test: %s\n", !strcmp(c->key, "test-first") ? "succeeded" : "failed");
	xfire_free(c);

	for(i = 0; i < 5; i++) {
		c = bio_queue_pop();
		xfire_free(c);
	}
}
#endif

