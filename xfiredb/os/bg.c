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

/**
 * @addtogroup bg
 * @{
 */

#include <stdlib.h>
#include <stdio.h>

#include <xfire/xfire.h>
#include <xfire/types.h>
#include <xfire/bg.h>
#include <xfire/mem.h>
#include <xfire/os.h>
#include <xfire/error.h>
#include <xfire/dict.h>

static struct dict *job_db;
static size_t bg_proc_stack = 0;

/**
 * @brief Initialise the background processing module.
 */
void bg_processes_init(void)
{
	size_t stack;
	xfire_attr_t attr;

	job_db = dict_alloc();
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

		j->handle(j->arg);
		if(j->done)
			break;
	}

	xfire_thread_exit(NULL);
}

/**
 * @brief Create a new background job.
 * @param name Name of the job to create.
 * @param handle Job handler (function pointer).
 * @param arg Argument to \p handle.
 * @note The \p name argument has to be unique.
 */
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
	dict_add(job_db, name, job, DICT_PTR);
	job->stamp = time(NULL);

	job->tp = __xfire_create_thread(name, &bg_proc_stack, &job_processor, job);
	if(!job->tp)
		return NULL;

	return job;
}

/**
 * @brief Signal a sleeping job.
 * @param name Name of the job to signal.
 *
 * When a job with the name \p name has been found, it will
 * be woken up, if possible at all.
 */
int bg_process_signal(const char *name)
{
	union entry_data data;
	size_t tmp;
	struct job *job;

	if(dict_lookup(job_db, name, &data, &tmp))
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

/**
 * @brief Stop a running (or sleeping) job.
 * @param name Name of the job which has to be stopped.
 */
int bg_process_stop(const char *name)
{
	union entry_data data;
	struct job *job;

	if(dict_delete(job_db, name, &data, false))
		return -XFIRE_ERR;

	job = data.ptr;
	return __bg_process_stop(job);
}

/**
 * @brief Stop the background process module.
 */
void bg_processes_exit(void)
{
	struct job *job;
	struct dict_iterator *it;
	struct dict_entry *e;
	union entry_data data;
	int rv;

	it = dict_get_iterator(job_db);
	e = dict_iterator_next(it);

	while(e) {
		rv = dict_delete(job_db, e->key, &data, false);
		if(rv)
			continue;
		job = data.ptr;
		if(!job)
			continue;

		__bg_process_stop(job);
		e = dict_iterator_next(it);
	}

	dict_iterator_free(it);
	dict_free(job_db);
	return;
}

/** @} */

