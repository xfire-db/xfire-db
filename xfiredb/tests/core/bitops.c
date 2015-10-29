/*
 *  Binary operations
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

#include <xfiredb/engine/types.h>
#include <xfiredb/engine/bitops.h>
#include <asm/bitsperlong.h>

#define RESULT_OK "OK"
#define RESULT_ERROR "ERROR"

static int bits[] = {__BITS_PER_LONG, 5, 7, 14, 21, 30};
static unsigned long flags1 = (1UL << 5) |
			  (1UL << 7) |
			  (1UL << 14) |
			  (1UL << 21) |
			  (1UL << 30);
static unsigned long flags2 = 1UL;

static void set_bit_test(int *bits, unsigned long *flags, int bit_num)
{
	int i;

	for(i = 0; i < bit_num; i++)
		__set_bit(bits[i], flags);

	if(flags[0] == flags1 && flags[1] == flags2)
		printf("[OK] set_bit\n");
	else
		printf("[ERROR] set_bit %lu::%lu\n", flags[0], flags[1]);
}

static void test_and_clear_bit_test(int *bits, unsigned long *flags, int num)
{
	int i,
	    result = 1;

	for(i = 0; i < num; i++)
		result &= __test_and_clear_bit(bits[i], flags);

	if(result && (flags[0] == 0UL && flags[1] == 0UL))
		printf("[OK] test_and_clear_bit\n");
	else
		printf("[ERROR] test_and_clear_bit %lu::%lu\n", 
				flags[0], flags[1]);

}

static void test_and_set_bit_test(int *bits, unsigned long *flags, int num)
{
	int i,
	    result = 1;

	for(i = 0; i < num; i++)
		result &= !__test_and_set_bit(bits[i], flags);

	if(result && (flags[0] == flags1 && flags[1] == flags2))
		printf("[OK] test_and_set_bit\n");
	else
		printf("[ERROR] test_and_set_bit %lu::%lu\n", 
				flags[0], flags[1]);
}

static void clear_bit_test(int *bits, unsigned long *flags, int num)
{
	int i;

	for(i = 0; i < num; i++)
		__clear_bit(bits[i], flags);

	if(flags[0] == 0UL && flags[1] == 0UL)
		printf("[OK] set_bit\n");
	else
		printf("[ERROR] set_bit %lu::%lu\n", 
				flags[0], flags[1]);
}

static void test_bit_test(int *bits, unsigned long *flags, int bit_num)
{
	int i,
	    result = 1;

	for(i = 0; i < bit_num; i++)
		result &= __test_bit(bits[i], flags);

	printf("[%s] test_bit\n", result ? RESULT_OK : RESULT_ERROR);
}

static unsigned long x1 = 9UL,
		     y1 = 14UL,
		     x2 = 14UL,
		     y2 = 9UL,
		     x3 = 11UL,
		     y3 = 6UL,
		     x4 = 9UL,
		     y4 = 8UL;
#define SWAP_BIT_BIT 1

#define X1_RESULT 11UL
#define Y1_RESULT 12UL

#define X2_RESULT 12UL
#define Y2_RESULT 11UL

#define X3_RESULT 11UL
#define Y3_RESULT 6UL

#define X4_RESULT 9UL
#define Y4_RESULT 8UL

static void swap_bit_test(void)
{
	bool result = true;

	__swap_bit(SWAP_BIT_BIT, &x1, &y1);
	__swap_bit(SWAP_BIT_BIT, &x2, &y2);
	__swap_bit(SWAP_BIT_BIT, &x3, &y3);
	__swap_bit(SWAP_BIT_BIT, &x4, &y4);

	if(x1 != X1_RESULT || y1 != Y1_RESULT) {
		printf("[ERROR]: x1 %lu ?= %lu :: y1 %lu ?= %lu\n",
				x1, X1_RESULT,
				y1, Y1_RESULT);
		result = false;
	}

	if(x2 != X2_RESULT || y2 != Y2_RESULT) {
		printf("[ERROR]swap_bit: x2 %lu ?= %lu :: y2 %lu ?= %lu\n",
				x2, X2_RESULT,
				y2, Y2_RESULT);
		result = false;
	}

	if(x3 != X3_RESULT || y3 != Y3_RESULT) {
		printf("[ERROR]swap_bit: x3 %lu ?= %lu :: y3 %lu ?= %lu\n",
				x3, X3_RESULT,
				y3, Y3_RESULT);
		result = false;
	}

	if(x4 != X4_RESULT || y4 != Y4_RESULT) {
		printf("[ERROR]swap_bit: x4 %lu ?= %lu :: y4 %lu ?= %lu\n",
				x4, X4_RESULT,
				y4, Y4_RESULT);
		result = false;
	}

	if(result)
		printf("[OK] swap_bit\n");
}

int main(int argc, char **argv)
{
	unsigned long flags[2] = {0UL, 0UL};
	int num = sizeof(bits) / sizeof(*bits);

	set_bit_test(bits, flags, num);
	test_bit_test(bits, flags, num);
	test_and_clear_bit_test(bits, flags, num);
	test_and_set_bit_test(bits, flags, num);
	clear_bit_test(bits, flags, num);
	swap_bit_test();

	putc('\n', stdout);
	printf("All tests executed!\n");
	return -EXIT_SUCCESS;
}

