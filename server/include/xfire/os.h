/*
 *  OS library
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

#ifndef __XFIRE_OS__
#define __XFIRE_OS__

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <xfire/xfire.h>

struct thread {
	pthread_t thread;
	pthread_attr_t attr;

	char *name;
};

CDECL
extern pthread_t *xfire_create_thread(const char *name,
				      const pthread_attr_t *attr, 
				      void* (*fn)(void*),
				      void* arg);
extern void *xfire_thread_join(pthread_t *tp);
extern int xfire_destroy_thread(pthread_t *tp);

static inline void *mzalloc(size_t size)
{
	void *rv;

	if(!size)
		return NULL;

	rv = malloc(size);
	
	if(!rv)
		return NULL;

	memset(rv, 0x0, size);
	return rv;
}
CDECL_END

#endif
