/*
 *  XFireDB quotearg unit test
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
#include <stdio.h>
#include <unittest.h>

#include <xfiredb/xfiredb.h>
#include <xfiredb/types.h>
#include <xfiredb/quotearg.h>

void setup(void)
{
}

void teardown(void)
{
}

void test_quotearg(void)
{
	char *esc1, *esc2, *uesc1, *uesc2;
	char *str1 = "Hey I'm \"Julie Edwards\"";
	char *str2 = "Testing\nnewline\nescapes";

	esc1 = xfiredb_escape_string(str1);
	esc2 = xfiredb_escape_string(str2);

	puts(esc1);
	puts(esc2);

	uesc1 = xfiredb_unescape_string(esc1);
	uesc2 = xfiredb_unescape_string(esc2);

	puts(uesc1);
	puts(uesc2);

	xfiredb_escape_free(esc1);
	xfiredb_escape_free(esc2);
	xfiredb_escape_free(uesc1);
	xfiredb_escape_free(uesc2);
}

test_func_t test_func_array[] = {test_quotearg, NULL};
const char *test_name = "Quotearg unit test";

