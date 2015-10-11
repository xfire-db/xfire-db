/*
 *  Background processes
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
#include <xfire/mem.h>
#include <xfire/os.h>
#include <xfire/error.h>
#include <xfire/database.h>

static struct database *job_db;
static size_t bg_proc_stack = 0;

void bg_processes_init(void)
{
	size_t stack;
	xfire_attr_t attr;

	job_db = db_alloc("job-db");
	xfire_attr_init(&attr);
	xfire_get_stack_size(&attr, &stack);

	if(!stack) /* solaris is stupid and sets this to 0 */
		stack = 1UL;

	while(stack < XFIRE_STACK_SIZE)
		stack *= 2;
	
	bg_proc_stack = stack;
}

static void *job_processor(void *arg)
{
	struct job *j = arg;

	while(true) {
		xfire_mutex_lock(&j->lock);
		if(!j->done)
			xfire_cond_wait(&j->condi, &j->lock);
		xfire_mutex_unlock(&j->lock);

		if(j->done)
			break;
		else
			j->handle(arg);
	}

	xfire_thread_exit(NULL);
}

struct job *bg_process_create(const char *name,
				void (*handle)(void*), void *arg)
{
	int l;
	char *_name;
	struct job *job;

	l = strlen(name);
	_name = xfire_zalloc(l + 1);
	memcpy(_name, name, l);

	job = xfire_zalloc(sizeof(*job));
	job->name = _name;
	job->handle = handle;
	job->arg = arg;
	xfire_mutex_init(&job->lock);
	xfire_cond_init(&job->condi);
	db_store(job_db, name, job);
	job->stamp = time(NULL);

	job->tp = __xfire_create_thread(name, &bg_proc_stack, &job_processor, job);
	if(!job->tp)
		return NULL;

	return job;
}

int bg_process_signal(const char *name)
{
	db_data_t data;
	struct job *job;

	if(db_lookup(job_db, name, &data))
		return -XFIRE_ERR;

	job = data.ptr;
	if(!job)
		return -XFIRE_ERR;

	xfire_mutex_lock(&job->lock);
	xfire_cond_signal(&job->condi);
	xfire_mutex_unlock(&job->lock);

	return -XFIRE_OK;
}

static int __bg_process_stop(struct job *job)
{
	if(!job)
		return -XFIRE_ERR;

	xfire_mutex_lock(&job->lock);
	job->done = true;
	xfire_cond_signal(&job->condi);
	xfire_mutex_unlock(&job->lock);

	xfire_thread_join(job->tp);
	xfire_thread_destroy(job->tp);
	xfire_cond_destroy(&job->condi);
	xfire_mutex_destroy(&job->lock);

	xfire_free(job->name);
	xfire_free(job);

	return -XFIRE_OK;
}

int bg_process_stop(const char *name)
{
	db_data_t data;
	struct job *job;

	if(db_delete(job_db, name, &data))
		return -XFIRE_ERR;

	job = data.ptr;
	return __bg_process_stop(job);
}

void bg_processes_exit(void)
{
	struct job *job;
	struct db_iterator *it;
	struct db_entry *e;
	db_data_t data;
	int rv;

	it = db_get_iterator(job_db);
	e = db_iterator_next(it);

	while(e) {
		rv = db_delete(job_db, e->key, &data);
		if(rv)
			continue;
		job = data.ptr;
		if(!job)
			continue;

		__bg_process_stop(job);
		e = db_iterator_next(it);
	}

	db_iterator_free(it);
	db_free(job_db);
	return;
}

