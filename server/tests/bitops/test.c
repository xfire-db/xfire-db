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

#include <xfire/bitops.h>
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
		set_bit(bits[i], flags);

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
		result &= test_and_clear_bit(bits[i], flags);

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
		result &= !test_and_set_bit(bits[i], flags);

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
		clear_bit(bits[i], flags);

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
		result &= test_bit(bits[i], flags);

	printf("[%s] test_bit\n", result ? RESULT_OK : RESULT_ERROR);
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

	putc('\n', stdout);
	printf("All tests executed!\n");
	return -EXIT_SUCCESS;
}

