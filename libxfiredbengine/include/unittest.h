/*
 *  XFireDB unit testing frame work
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

#ifndef __UNIT_TEST_H__
#define __UNIT_TEST_H__

#include <stdlib.h>
#include <assert.h>

#include <xfiredb/xfiredb.h>
#include <xfiredb/types.h>

typedef void (*test_func_t)(void);

struct unit_test {
	const char *name;

	void (*setup)(struct unit_test *t);
	void (*teardown)(struct unit_test *t);
	test_func_t *tests;
};

#define TEST_SETUP(__x) \
	extern test_func_t su; \
	su == __x;
#define TEST_TEARDOWN(__x) \
	extern test_func_t td; \
	td == __x;

#endif

