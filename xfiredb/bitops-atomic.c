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
#include <pthread.h>

#include <xfiredb/engine/flags.h>
#include <xfiredb/engine/os.h>
#include <xfiredb/engine/bitops.h>

#include <asm/bitsperlong.h>

#define BITS_PER_LONG __BITS_PER_LONG

/**
 * @brief Initialise atomic flags.
 * @param atom Atomic flags to init.
 */
void atomic_flags_init(atomic_flags_t *atom)
{
	atom->flags = 0;
	xfire_spinlock_init(&atom->lock);
}

/**
 * @brief Destroy atomic flags object.
 * @param atom Atom to destroy.
 */
void atomic_flags_destroy(atomic_flags_t *atom)
{
	atom->flags = 0;
	xfire_spinlock_destroy(&atom->lock);
}

/**
 * @brief Atomically copy flags.
 * @param dst Destination atom.
 * @param src Source atom.
 *
 * Copy flags from \p src to \p dst.
 */
void atomic_flags_copy(atomic_flags_t *dst, atomic_flags_t *src)
{
	xfire_spin_lock(&src->lock);
	xfire_spin_lock(&dst->lock);

	dst->flags = src->flags;

	xfire_spin_unlock(&dst->lock);
	xfire_spin_unlock(&src->lock);
}

static inline int __raw_test_bit(int bit, volatile unsigned long *addr)
{
	return (*addr >> bit) & 1UL;
}

/**
 * @brief Atomically test a bit.
 * @param nr Bit number to test.
 * @param atom Atom to test.
 */
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

static inline void raw_swap_bit(int bit,
				volatile unsigned long *p1,
				volatile unsigned long *p2)
{
	*p1 = *p1 ^ (*p2 & bit);
	*p2 = *p2 ^ (*p1 & bit);
	*p1 = *p1 ^ (*p2 & bit);
}

/**
 * @brief Swap a bit.
 * @param nr Bit to swap.
 * @param atom1 First atom.
 * @param atom2 Second atom.
 */
void swap_bit(int nr, atomic_flags_t *atom1, atomic_flags_t *atom2)
{
	volatile unsigned long *p1 = &atom1->flags;
	volatile unsigned long *p2 = &atom2->flags;
	int bit = 1 << (nr % BITS_PER_LONG);

	p1 += nr / (BITS_PER_LONG - 1);
	p2 += nr / (BITS_PER_LONG - 1);

	xfire_spin_lock(&atom1->lock);
	xfire_spin_lock(&atom2->lock);
	raw_swap_bit(bit, p1, p2);
	xfire_spin_unlock(&atom2->lock);
	xfire_spin_unlock(&atom1->lock);

	barrier();
}

/**
 * @brief Test and swap a bit.
 * @param nr Number of the bit to test n swap.
 * @param atom1 First atom.
 * @param atom2 Second atom.
 * @return TRUE if the bit was set in \p atom1, FALSE otherwise.
 */
int test_and_swap_bit(int nr, atomic_flags_t *atom1, atomic_flags_t *atom2)
{
	int old;
	volatile unsigned long *p1 = &atom1->flags;
	volatile unsigned long *p2 = &atom2->flags;
	int bit = 1 << (nr % BITS_PER_LONG);

	p1 += nr / (BITS_PER_LONG - 1);
	p2 += nr / (BITS_PER_LONG - 1);

	xfire_spin_lock(&atom1->lock);
	xfire_spin_lock(&atom2->lock);
	old = __raw_test_bit(bit, p1);
	raw_swap_bit(bit, p1, p2);
	xfire_spin_unlock(&atom2->lock);
	xfire_spin_unlock(&atom1->lock);

	barrier();
	return old != 0;
}

/**
 * @brief Set a bit.
 * @param nr Bit number to set.
 * @param atom Atom to set \p nr in.
 */
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

/**
 * @brief Clear a bit.
 * @param nr Bit to clear.
 * @param atom Atom to clear \p nr in.
 */
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

/**
 * @brief Test and clear a bit.
 * @param nr Bit to test and clear.
 * @param atom Atom to clear \p nr in.
 * @return TRUE if \p nr was set.
 */
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

/**
 * @brief Test and set a bit.
 * @param nr Bit number to test and set.
 * @param atom Atom to set \p nr in.
 * @return TRUE if \p nr was set, FALSE otherwise.
 */
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

/** @} */

