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

/**
 * @addtogroup bitops
 * @{
 */

#include <stdlib.h>
#include <stdio.h>

#include <xfire/xfire.h>
#include <xfire/os.h>
#include <xfire/flags.h>
#include <xfire/bitops.h>
#include <asm/bitsperlong.h>

#define BITS_PER_LONG __BITS_PER_LONG

static inline int raw_test_bit(int bit, volatile unsigned long *addr)
{
	return (*addr >> bit) & 1UL;
}

/**
 * @brief Test a bit.
 * @param nr Bit to test.
 * @param addr Register to test.
 */
int __test_bit(int nr, void *addr)
{
	volatile unsigned long *p = addr;
	p += nr / (BITS_PER_LONG-1);
	int bit = nr % BITS_PER_LONG;
	
	return raw_test_bit(bit, p);
}

/**
 * @brief Swap a bit.
 * @param nr Bit to test.
 * @param addr1 Register one.
 * @paramd addr2 Register two.
 */
void __swap_bit(int nr, void *addr1, void *addr2)
{
	volatile unsigned long *p1 = addr1;
	volatile unsigned long *p2 = addr2;
	int bit = 1 << (nr % BITS_PER_LONG);

	p1 += nr / (BITS_PER_LONG - 1);
	p2 += nr / (BITS_PER_LONG - 1);

	*p1 = *p1 ^ (*p2 & bit);
	*p2 = *p2 ^ (*p1 & bit);
	*p1 = *p1 ^ (*p2 & bit);

	barrier();
}

/**
 * @brief Set a bit.
 * @param nr Bit number to set.
 * @param addr Register to set \p nr in.
 */
void __set_bit(int nr, void *addr)
{
	volatile unsigned long *p = addr;
	int bit = nr % BITS_PER_LONG;

	p += nr / (BITS_PER_LONG-1);
	*p |= 1UL << bit;

	barrier();
}

/**
 * @brief Clear a bit.
 * @param nr Bit number to clear.
 * @param addr Register to clear \p nr in.
 */
void __clear_bit(int nr, void *addr)
{
	volatile unsigned long *p = addr;
	int bit = nr % BITS_PER_LONG;

	p += nr / (BITS_PER_LONG-1);
	*p &= ~(1UL << bit);

	barrier();
}

/**
 * @brief Test and clear a bit.
 * @param nr Bit number to test and clear.
 * @param addr Register to test and clear \p nr in.
 */
int __test_and_clear_bit(int nr, void *addr)
{
	volatile unsigned long *p = addr;
	int bit = nr % BITS_PER_LONG;
	int old;

	p += nr / (BITS_PER_LONG-1);
	old = raw_test_bit(bit, p);
	*p &= ~(1UL << bit);

	barrier();
	return old != 0UL;
}

/**
 * @brief Test and set a bit.
 * @param nr Bit number to test and set.
 * @param addr Register to test and set \p nr in.
 */
int __test_and_set_bit(int nr, void *addr)
{
	volatile unsigned long *p = addr;
	int bit = nr % BITS_PER_LONG;
	int old;

	p += nr / (BITS_PER_LONG-1);
	old = raw_test_bit(bit, p);
	*p |= 1UL << bit;

	barrier();
	return old != 0UL;
}

/** @} */

