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

#include <xfire/xfire.h>
#include <xfire/os.h>

pthread_t *xfire_create_thread(const char *name, const pthread_attr_t *attr,
				void* (*fn)(void*),
				void* arg)
{
	struct thread *tp;

	tp = mzalloc(sizeof(*tp));
	pthread_attr_init(&tp->attr);
	pthread_attr_setdetachstate(&tp->attr, PTHREAD_CREATE_JOINABLE);
	
	if(pthread_create(&tp->thread, attr, fn, arg)) {
		free(tp);
		return NULL;
	}

	tp->name = mzalloc(strlen(name) + 1);
	memcpy(tp->name, name, strlen(name) + 1);

	return &tp->thread;
}

int xfire_destroy_thread(pthread_t *tid)
{
	struct thread *tp;

	tp = container_of(tid, struct thread, thread);
	
	/*
	 * free the allocated memory inside `tp'
	 */
	free(tp->name);
	free(tp);

	return -EXIT_SUCCESS;
}

void *xfire_thread_join(pthread_t *tid)
{
	void *rv;

	if(pthread_join(*tid, &rv))
		rv = NULL;

	return rv;
}
