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
#include <xfire/types.h>
#include <xfire/os.h>

struct thread *xfire_create_thread(const char *name, const pthread_attr_t *attr,
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

	return tp;
}

int xfire_destroy_thread(struct thread *tp)
{
	/*
	 * free the allocated memory inside `tp'
	 */
	free(tp->name);
	free(tp);

	return -EXIT_SUCCESS;
}

void *xfire_thread_join(struct thread *tp)
{
	void *rv;

	if(pthread_join(tp->thread, &rv))
		rv = NULL;

	return rv;
}

void atomic_add(atomic_t *atom, s32 val)
{
	xfire_spin_lock(&atom->lock);
	atom->val += val;
	xfire_spin_unlock(&atom->lock);
}

void atomic_sub(atomic_t *atom, s32 val)
{
	xfire_spin_lock(&atom->lock);
	atom->val -= val;
	xfire_spin_unlock(&atom->lock);
}

s32 atomic_get(atomic_t *atom)
{
	s32 tmp;

	xfire_spin_lock(&atom->lock);
	tmp = atom->val;
	xfire_spin_unlock(&atom->lock);

	return tmp;
}

void atomic64_add(atomic64_t *atom, s64 val)
{
	xfire_spin_lock(&atom->lock);
	atom->val += val;
	xfire_spin_unlock(&atom->lock);
}

void atomic64_sub(atomic64_t *atom, s64 val)
{
	xfire_spin_lock(&atom->lock);
	atom->val -= val;
	xfire_spin_unlock(&atom->lock);
}

s64 atomic64_get(atomic64_t *atom)
{
	s64 tmp;

	xfire_spin_lock(&atom->lock);
	tmp = atom->val;
	xfire_spin_unlock(&atom->lock);

	return tmp;
}

