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
#include <pthread.h>

#include <xfire/flags.h>
#include <xfire/os.h>
#include <xfire/bitops.h>

#include <asm/bitsperlong.h>

#define BITS_PER_LONG __BITS_PER_LONG

void atomic_flags_init(atomic_flags_t *atom)
{
	atom->flags = 0;
	xfire_spinlock_init(&atom->lock);
}

void atomic_flags_destroy(atomic_flags_t *atom)
{
	atom->flags = 0;
	xfire_spinlock_destroy(&atom->lock);
}

static inline int __raw_test_bit(int bit, volatile unsigned long *addr)
{
	return (*addr >> bit) & 1UL;
}

int test_bit(int nr, atomic_flags_t *atom)
{
	volatile unsigned long *p = &atom->flags;
	p += nr / (BITS_PER_LONG-1);
	int bit = nr % BITS_PER_LONG, rv;

	xfire_spin_lock(&atom->lock);
	rv = __raw_test_bit(bit, p);
	xfire_spin_unlock(&atom->lock);

	return rv;
}

void swap_bit(int nr, atomic_flags_t *atom1, atomic_flags_t *atom2)
{
	volatile unsigned long *p1 = &atom1->flags;
	volatile unsigned long *p2 = &atom2->flags;
	int bit = 1 << (nr % BITS_PER_LONG);

	p1 += nr / (BITS_PER_LONG - 1);
	p2 += nr / (BITS_PER_LONG - 1);

	xfire_spin_lock(&atom1->lock);
	xfire_spin_lock(&atom2->lock);
	*p1 = *p1 ^ (*p2 & bit);
	*p2 = *p2 ^ (*p1 & bit);
	*p1 = *p1 ^ (*p2 & bit);
	xfire_spin_unlock(&atom2->lock);
	xfire_spin_unlock(&atom1->lock);

	barrier();
}

void set_bit(int nr, atomic_flags_t *atom)
{
	volatile unsigned long *p = &atom->flags;
	int bit = nr % BITS_PER_LONG;

	p += nr / (BITS_PER_LONG-1);
	xfire_spin_lock(&atom->lock);
	*p |= 1UL << bit;
	xfire_spin_unlock(&atom->lock);

	barrier();
}

void clear_bit(int nr, atomic_flags_t *atom)
{
	volatile unsigned long *p = &atom->flags;
	int bit = nr % BITS_PER_LONG;

	p += nr / (BITS_PER_LONG-1);

	xfire_spin_lock(&atom->lock);
	*p &= ~(1UL << bit);
	xfire_spin_unlock(&atom->lock);

	barrier();
}

int test_and_clear_bit(int nr, atomic_flags_t *atom)
{
	volatile unsigned long *p = &atom->flags;
	int bit = nr % BITS_PER_LONG;
	int old;

	p += nr / (BITS_PER_LONG-1);
	xfire_spin_lock(&atom->lock);
	old = __raw_test_bit(bit, p);
	*p &= ~(1UL << bit);
	xfire_spin_unlock(&atom->lock);

	barrier();
	return old != 0UL;
}

int test_and_set_bit(int nr, atomic_flags_t *atom)
{
	volatile unsigned long *p = &atom->flags;
	int bit = nr % BITS_PER_LONG;
	int old;

	p += nr / (BITS_PER_LONG-1);
	xfire_spin_lock(&atom->lock);
	old = __raw_test_bit(bit, p);
	*p |= 1UL << bit;
	xfire_spin_unlock(&atom->lock);

	barrier();
	return old != 0UL;
}
