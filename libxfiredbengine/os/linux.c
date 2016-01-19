/*
 *  Linux interface
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
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <math.h>

#include <xfiredb/xfiredb.h>
#include <xfiredb/error.h>
#include <xfiredb/types.h>
#include <xfiredb/os.h>
#include <xfiredb/mem.h>

void xfiredb_mutex_init(xfiredb_mutex_t *m)
{
	pthread_mutexattr_init(&m->attr);
	pthread_mutexattr_settype(&m->attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&m->mtx, &m->attr);
}

void xfiredb_mutex_destroy(xfiredb_mutex_t *m)
{
	pthread_mutex_destroy(&m->mtx);
	pthread_mutexattr_destroy(&m->attr);
}

void xfiredb_mutex_lock(xfiredb_mutex_t *m)
{
	pthread_mutex_lock(&m->mtx);
}

void xfiredb_mutex_unlock(xfiredb_mutex_t *m)
{
	pthread_mutex_unlock(&m->mtx);
}

struct thread *__xfiredb_create_thread(const char *name,
				size_t *stack,
				void *(*fn)(void*),
				void *arg)
{
	struct thread *tp;

	tp = xfiredb_zalloc(sizeof(*tp));
	pthread_attr_init(&tp->attr);

	if(stack)
		xfiredb_set_stack_size(&tp->attr, stack);

	pthread_attr_setdetachstate(&tp->attr, PTHREAD_CREATE_JOINABLE);
	
	if(pthread_create(&tp->thread, &tp->attr, fn, arg)) {
		free(tp);
		return NULL;
	}

	tp->name = xfiredb_zalloc(strlen(name) + 1);
	memcpy(tp->name, name, strlen(name));

	return tp;
}

struct thread *xfiredb_create_thread(const char *name,
				void* (*fn)(void*),
				void* arg)
{
	return __xfiredb_create_thread(name, NULL, fn, arg);
}

int xfiredb_thread_cancel(struct thread *tp)
{
	return pthread_cancel(tp->thread);
}

int xfiredb_destroy_thread(struct thread *tp)
{
	/*
	 * free the allocated memory inside `tp'
	 */
	free(tp->name);
	free(tp);

	return -XFIREDB_OK;
}

void *xfiredb_thread_join(struct thread *tp)
{
	void *rv;

	if(pthread_join(tp->thread, &rv))
		rv = NULL;

	return rv;
}

time_t xfiredb_time_stamp(void)
{
	time_t rv;
	long s, ms;
	struct timespec spec;

	clock_gettime(CLOCK_REALTIME, &spec);
	s = spec.tv_sec;
	ms = round(spec.tv_nsec / 1.0e6);
	rv = s * 1000;
	rv += ms;

	return rv;
}

