/*
 *  XFire client
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

#ifndef __XFIRE_CLIENT_H_
#define __XFIRE_CLIENT_H_

#include <config.h>

#ifdef __cplusplus
class XFire {
public:
	explicit XFire(const char *addr, int port);
	virtual ~XFire();
};
#endif

#ifdef __cplusplus
#define CDECL extern "C" {
#define CDECL_END }
#else
#define CDECL
#define CDECL_END
#endif

#ifndef __cplusplus
#ifndef true
#define true 1
#endif

#ifndef false
#define false !true
#endif

#endif

#define XOR(a,b) ((a) ^ (b))

#ifdef __GNUC__
#define __compiler_offsetof(a,b) __builtin_offsetof(a,b)
#define offsetof(a,b) __compiler_offsetof(a,b)
#else
#define offsetof(a,b) ((size_t) &((a *)0)->b)
#endif

#define container_of(ptr, type, member) ({		\
		const typeof( ((type *)0)->member) *__mptr = (ptr); \
		(type *)( ( char *)__mptr - offsetof(type,member) );})

CDECL
extern int xfire_sprintf(char **buf, char *format, ...);
CDECL_END

#endif
