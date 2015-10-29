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

/**
 * @addtogroup thread
 * @{
 */

#ifndef __XFIRE_OS__
#define __XFIRE_OS__

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <xfiredb/xfiredb.h>
#include <xfiredb/types.h>

/**
 * @brief Pointer size.
 */
#define PTR_SIZE (sizeof(void*))

#if defined(HAVE_LINUX) || defined(__DOXYGEN__)
#define xfire_cond_t pthread_cond_t //!< XFire condition variable.
#define xfire_mutex_t mutex_t //!< XFire mutex.
#define xfire_spinlock_t pthread_spinlock_t //!< XFire spinlock.
#define xfire_attr_t pthread_attr_t //!< Thread attributes
#define xfire_attr_init(_atr) pthread_attr_init(_atr) //!< Init thread attributes

/**
 * @brief Get the stack size.
 * @param attr Thread attributes.
 * @param stack Pointer to store the stack size in.
 */
static inline void xfire_get_stack_size(xfire_attr_t *attr, size_t *stack)
{
	pthread_attr_getstacksize(attr, stack);
}

/**
 * @brief Set the stack size.
 * @param attr Attributes to set the stack size for.
 * @param stack Stack size to set.
 */
static inline void xfire_set_stack_size(xfire_attr_t *attr, size_t *stack)
{
	pthread_attr_setstacksize(attr, *stack);
}

/**
 * @brief Initialise a spinlock.
 * @param __s Spinlock to initialise.
 */
#define xfire_spinlock_init(__s) pthread_spin_init(__s, PTHREAD_PROCESS_PRIVATE)
/**
 * @brief Destroy a spinlock.
 * @param __s Spin lock to destroy.
 */
#define xfire_spinlock_destroy(__s) pthread_spin_destroy(__s)
/**
 * @brief Lock a spinlock.
 * @param __s Spinlock to lock.
 */
#define xfire_spin_lock(__s) pthread_spin_lock(__s)
/**
 * @brief Attempt locking a spinlock.
 * @param __s Spinlock to try locking.
 *
 * If this function is unable to lock \p __s, it will return control
 * to the caller.
 */
#define xfire_spin_trylock(__s) pthread_spin_trylock(__s)

/**
 * @brief Unlock a spinlock.
 * @param __s Spinlock to unlock.
 */
#define xfire_spin_unlock(__s) pthread_spin_unlock(__s)

/**
 * @brief Initialise a condition variable.
 * @param __c Condition variable to init.
 */
#define xfire_cond_init(__c) pthread_cond_init(__c, NULL)
/**
 * @brief Destroy a condition variable.
 * @param __c Variable to destroy.
 */
#define xfire_cond_destroy(__c) pthread_cond_destroy(__c)
/**
 * @brief Wait for a condition to come true.
 * @param __c Condition to wait for.
 * @param __m Mutex to use for the waiting process.
 */
#define xfire_cond_wait(__c, __m) pthread_cond_wait(__c, &(__m)->mtx)
/**
 * @brief Signal a condition.
 * @param __c Condition to signal.
 */
#define xfire_cond_signal(__c) pthread_cond_signal(__c)
/**
 * @brief Exit a thread.
 * @param __a Thread to exit.
 */
#define xfire_thread_exit(__a) pthread_exit(__a)
#endif

/**
 * @brief Kill a thread.
 * @param __tp Thread to kill.
 */
#define xfire_thread_destroy(__tp) xfire_destroy_thread(__tp)

/**
 * @brief Custom thread structure.
 */
struct thread {
#if defined(HAVE_LINUX) || defined(__DOXYGEN__)
	pthread_t thread; //!< pthread thread.
	pthread_attr_t attr; //!< pthread attributes.
#endif

	char *name; //!< Thread name.
};

/**
 * @brief Mutex data structure.
 */
typedef struct mutex {
#if defined(HAVE_LINUX) || defined(__DOXYGEN__)
	pthread_mutex_t mtx; //!< pthread mutex
	pthread_mutexattr_t attr; //!< pthread mutex attributes
#endif
} mutex_t;

/**
 * @brief 32-bit atomic type.
 */
typedef struct atomic {
	s32 val; //!< Atomic value.
	xfire_spinlock_t lock; //!< Protection lock.
} atomic_t;

/**
 * @brief 64-bit atomic type.
 */
typedef struct atomic64 {
	s64 val; //!< 64-bit atomic type.
	xfire_spinlock_t lock; //!< Protection lock.
} atomic64_t;

/**
 * @brief Increase an atomic value.
 * @param __a Atom to increase.
 */
#define atomic_inc(__a) atomic_add(&__a, 1)

/**
 * @brief Decrease an atomic value.
 * @param __a Atom to decrease.
 */
#define atomic_dec(__a) atomic_sub(&__a, 1)

/**
 * @brief Increase an atomic value.
 * @param __a Atom to increase.
 */
#define atomic64_inc(__a) atomic64_add(&__a, 1LL)

/**
 * @brief Decrease an atomic value.
 * @param __a Atom to decrease.
 */
#define atomic64_dec(__a) atomic64_sub(&__a, 1LL)

#ifdef __GNUC__
#define barrier() __sync_synchronize()
#else
#define barrier() asm volatile("" ::: "memory")
#endif

CDECL
/**
 * @brief Destroy a mutex.
 * @param m Mutex to destroy.
 */
extern void xfire_mutex_destroy(xfire_mutex_t *m);
/**
 * @brief Lock a mutex.
 * @param m Mutex to lock.
 */
extern void xfire_mutex_lock(xfire_mutex_t *m);
/**
 * @brief Unlock a mutex.
 * @param m Mutex to unlock.
 */
extern void xfire_mutex_unlock(xfire_mutex_t *m);
/**
 * @brief Create a new thread.
 * @param name Thread name.
 * @param fn Thread function pointer.
 * @param arg Argument to be passed to the thread.
 * @return Created thread data structure.
 */
extern struct thread *xfire_create_thread(const char *name,
				      void* (*fn)(void*),
				      void* arg);
struct thread *__xfire_create_thread(const char *name,
				size_t *stack,
				void *(*fn)(void*),
				void *arg);
/**
 * @brief Join two threads.
 * @param tp Thread to join.
 */
extern void *xfire_thread_join(struct thread *tp);
/**
 * @brief Destroy a thread structure.
 * @param tp Thread structure to destroy.
 * @return An error code.
 */
extern int xfire_destroy_thread(struct thread *tp);
/**
 * @brief Stop a thread.
 * @param tp Thread to stop.
 * @return An error code.
 */
extern int xfire_thread_cancel(struct thread *tp);

/**
 * @brief Add a number atomically.
 * @param atom Atom to add to.
 * @param val Value to add.
 */
extern void atomic_add(atomic_t *atom, s32 val);

/**
 * @brief Substract a number atomically.
 * @param atom Atom to substract from.
 * @param val Value to substract.
 */
extern void atomic_sub(atomic_t *atom, s32 val);

/**
 * @brief Add a number atomically.
 * @param atom Atom to add to.
 * @param val Value to add.
 */
extern void atomic64_add(atomic64_t *atom, s64 val);

/**
 * @brief Substract a number atomically.
 * @param atom Atom to substract from.
 * @param val Value to substract.
 */
extern void atomic64_sub(atomic64_t *atom, s64 val);

/**
 * @brief Get a value atomically
 * @param atom Atom to retrieve the value from.
 * @return The value stored in \p atom.
 */
extern s32 atomic_get(atomic_t *atom);
/**
 * @brief Get a value atomically
 * @param atom Atom to retrieve the value from.
 * @return The value stored in \p atom.
 */
extern s64 atomic64_get(atomic64_t *atom);
/**
 * @brief Initialise a mutex variable.
 * @param m Mutex variable.
 */
extern void xfire_mutex_init(xfire_mutex_t *m);

/**
 * @brief Destroy a 32-bit atomic.
 * @param atom Atomic to kill.
 */
extern void atomic_destroy(atomic_t *atom);

/**
 * @brief Destroy a 64-bit atomic.
 * @param atom Atomic to kill.
 */
extern void atomic64_destroy(atomic64_t *atom);

/**
 * @brief Initialise an atomic variable.
 * @param atom Atom to init.
 */
static inline void atomic_init(atomic_t *atom)
{
	atom->val = 0;
	xfire_spinlock_init(&atom->lock);
}

/**
 * @brief Initialise an atomic variable.
 * @param atom Atom to init.
 */
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

/** @} */

