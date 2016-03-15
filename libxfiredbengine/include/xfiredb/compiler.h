/*
 *  Compiler header
 *  Copyright (C) 2016   Michel Megens <dev@michelmegens.net>
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

#ifndef __COMPILER_H__
#define __COMPILER_H__

#include <xfiredb/undef.h>

#ifdef __GNUC__
#include <xfiredb/compiler-gcc.h>
#else /* __GNUC__ */
#endif /* __GNUC__ */

#ifndef offsetof
#ifdef __compiler_offsetof
#define offsetof(a,b) __compiler_offsetof(a,b)
#else
#define offsetof(a,b) ((size_t) &((a *)0)->b)
#endif /* __compiler_offsetof */
#endif /*offsetof */

#ifndef container_of
#define container_of(ptr, type, member) ({		\
		const typeof( ((type *)0)->member) *__mptr = (ptr); \
		(type *)( ( char *)__mptr - offsetof(type,member) );})
#endif

#ifndef barrier
#define barrier() __asm__ __volatile("", ::: "memory")
#endif

#ifndef likely
#define likely(x) x
#endif

#ifndef unlikely
#define unlikely(x) x
#endif

#endif /* __COMPILER_H__ */

