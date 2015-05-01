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
#include <xfire/types.h>

#ifdef HAVE_LINUX
#define xfire_cond_t pthread_cond_t
#define xfire_mutex_t pthread_mutex_t
#define xfire_spinlock_t pthread_spinlock_t

#define xfire_spinlock_init(__s) pthread_spin_init(__s, PTHREAD_PROCESS_PRIVATE)
#define xfire_spinlock_destroy(__s) pthread_spin_destroy(__s)
#define xfire_spin_lock(__s) pthread_spin_lock(__s)
#define xfire_spin_trylock(__s) pthread_spin_trylock(__s)
#define xfire_spin_unlock(__s) pthread_spin_unlock(__s)

#define xfire_mutex_lock(__l) pthread_mutex_lock(__l)
#define xfire_mutex_unlock(__l) pthread_mutex_unlock(__l)
#define xfire_mutex_destroy(__l) pthread_mutex_destroy(__l)

#define xfire_cond_init(__c) pthread_cond_init(__c, NULL)
#define xfire_cond_destroy(__c) pthread_cond_destroy(__c)
#define xfire_cond_wait(__c, __m) pthread_cond_wait(__c, __m)
#define xfire_cond_signal(__c) pthread_cond_signal(__c)

#define xfire_thread_exit(__a) pthread_exit(__a)
#endif

#define xfire_thread_destroy(__tp) xfire_destroy_thread(__tp)

struct thread {
#ifdef HAVE_LINUX
	pthread_t thread;
	pthread_attr_t attr;
#endif

	char *name;
};

typedef struct atomic {
	s32 val;
	xfire_spinlock_t lock;
} atomic_t;

typedef struct atomic64 {
	s64 val;
	xfire_spinlock_t lock;
} atomic64_t;

#define atomic_inc(__a) atomic_add(&__a, 1)
#define atomic_dec(__a) atomic_sub(&__a, 1)

#define atomic64_inc(__a) atomic64_add(&__a, 1LL)
#define atomic64_dec(__a) atomic64_sub(&__a, 1LL)

#ifdef __GNUC__
#define barrier() __sync_synchronize()
#else
#define barrier() asm volatile("" ::: "memory")
#endif

CDECL
extern struct thread *xfire_create_thread(const char *name,
				      void* (*fn)(void*),
				      void* arg);
extern void *xfire_thread_join(struct thread *tp);
extern int xfire_destroy_thread(struct thread *tp);

extern void atomic_add(atomic_t *atom, s32 val);
extern void atomic_sub(atomic_t *atom, s32 val);
extern void atomic64_add(atomic64_t *atom, s64 val);
extern void atomic64_sub(atomic64_t *atom, s64 val);
extern s32 atomic_get(atomic_t *atom);
extern s64 atomic64_get(atomic64_t *atom);
extern void xfire_mutex_init(xfire_mutex_t *m);

static inline void atomic_init(atomic_t *atom)
{
	atom->val = 0;
	xfire_spinlock_init(&atom->lock);
}

static inline void atomic64_init(atomic64_t *atom)
{
	atom->val = 0LL;
	xfire_spinlock_init(&atom->lock);
}

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
