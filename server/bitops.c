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

#include <xfire/xfire.h>
#include <xfire/bitops.h>
#include <asm/bitsperlong.h>

#define BITS_PER_LONG __BITS_PER_LONG

static inline int __test_bit(int bit, unsigned long *addr)
{
	return (*addr >> bit) & 1UL;
}

int test_bit(int nr, void *addr)
{
	unsigned long *p = addr;
	p += nr / (BITS_PER_LONG-1);
	int bit = nr % BITS_PER_LONG;

	return __test_bit(bit, p);
}

void swap_bit(int nr, void *addr1, void *addr2)
{
	unsigned long *p1 = addr1,
		      *p2 = addr2;
	int bit = 1 << (nr % BITS_PER_LONG);

	p1 += nr / (BITS_PER_LONG - 1);
	p2 += nr / (BITS_PER_LONG - 1);

	*p1 = *p1 ^ (*p2 & bit);
	*p2 = *p2 ^ (*p1 & bit);
	*p1 = *p1 ^ (*p2 & bit);
}

void set_bit(int nr, void *addr)
{
	unsigned long *p = addr;
	int bit = nr % BITS_PER_LONG;

	p += nr / (BITS_PER_LONG-1);
	*p |= 1UL << bit;
}

void clear_bit(int nr, void *addr)
{
	unsigned long *p = addr;
	int bit = nr % BITS_PER_LONG;

	p += nr / (BITS_PER_LONG-1);
	*p &= ~(1UL << bit);
}

int test_and_clear_bit(int nr, void *addr)
{
	unsigned long *p = addr;
	int bit = nr % BITS_PER_LONG;
	int old;

	p += nr / (BITS_PER_LONG-1);
	old = __test_bit(bit, p);
	*p &= ~(1UL << bit);

	return old != 0UL;
}

int test_and_set_bit(int nr, void *addr)
{
	unsigned long *p = addr;
	int bit = nr % BITS_PER_LONG;
	int old;

	p += nr / (BITS_PER_LONG-1);
	old = __test_bit(bit, p);
	*p |= 1UL << bit;

	return old != 0UL;
}

