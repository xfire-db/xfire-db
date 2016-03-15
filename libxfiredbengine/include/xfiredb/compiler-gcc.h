/*
 *  GCC header
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

#ifndef __COMPILER_GCC_H__
#define __COMPILER_GCC_H__

#define __compiler_offsetof(a,b) __builtin_offsetof(a,b)
#define barrier() __sync_synchronize()

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikel(x) __builtin_expect(!!(x), 0)

#endif /* __COMPILER_GCC_H__ */

