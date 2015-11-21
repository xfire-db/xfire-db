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

#include <xfiredb/xfiredb.h>
#include <xfiredb/error.h>
#include <xfiredb/types.h>
#include <xfiredb/os.h>
#include <xfiredb/mem.h>

void xfire_mutex_init(xfire_mutex_t *m)
{
	pthread_mutexattr_init(&m->attr);
	pthread_mutexattr_settype(&m->attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&m->mtx, &m->attr);
}

void xfire_mutex_destroy(xfire_mutex_t *m)
{
	pthread_mutex_destroy(&m->mtx);
	pthread_mutexattr_destroy(&m->attr);
}

void xfire_mutex_lock(xfire_mutex_t *m)
{
	pthread_mutex_lock(&m->mtx);
}

void xfire_mutex_unlock(xfire_mutex_t *m)
{
	pthread_mutex_unlock(&m->mtx);
}

struct thread *__xfire_create_thread(const char *name,
				size_t *stack,
				void *(*fn)(void*),
				void *arg)
{
	struct thread *tp;

	tp = xfire_zalloc(sizeof(*tp));
	pthread_attr_init(&tp->attr);

	if(stack)
		xfire_set_stack_size(&tp->attr, stack);

	pthread_attr_setdetachstate(&tp->attr, PTHREAD_CREATE_JOINABLE);
	
	if(pthread_create(&tp->thread, &tp->attr, fn, arg)) {
		free(tp);
		return NULL;
	}

	tp->name = xfire_zalloc(strlen(name) + 1);
	memcpy(tp->name, name, strlen(name));

	return tp;
}

struct thread *xfire_create_thread(const char *name,
				void* (*fn)(void*),
				void* arg)
{
	return __xfire_create_thread(name, NULL, fn, arg);
}

int xfire_thread_cancel(struct thread *tp)
{
	return pthread_cancel(tp->thread);
}

int xfire_destroy_thread(struct thread *tp)
{
	/*
	 * free the allocated memory inside `tp'
	 */
	free(tp->name);
	free(tp);

	return -XFIRE_OK;
}

void *xfire_thread_join(struct thread *tp)
{
	void *rv;

	if(pthread_join(tp->thread, &rv))
		rv = NULL;

	return rv;
}

