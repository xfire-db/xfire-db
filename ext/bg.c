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
	return NULL;
}

int bg_process_create(const char *name, void (*handle)(void*))
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
	xfire_mutex_init(&job->lock);
	xfire_cond_init(&job->condi);
	db_store(job_db, name, job);
	job->stamp = time(NULL);

	job->tp = __xfire_create_thread(name, &bg_proc_stack, &job_processor, job);
	if(!job->tp)
		return -XFIRE_ERR;

	return -XFIRE_OK;
}

int bg_process_signal(const char *name)
{
	db_data_t data;
	struct job *job;

	if(db_lookup(job_db, name, &data))
		return -XFIRE_ERR;

	job = data.ptr;
	xfire_mutex_lock(&job->lock);
	xfire_cond_signal(&job->condi);
	xfire_mutex_unlock(&job->lock);

	return -XFIRE_OK;
}

int bg_process_stop(const char *name)
{
	db_data_t data;
	struct job *job;

	if(db_delete(job_db, name, &data))
		return -XFIRE_ERR;

	job = data.ptr;
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

