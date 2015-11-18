/*
 *  Atomic functions
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
#include <string.h>

#include <xfiredb/xfiredb.h>
#include <xfiredb/error.h>
#include <xfiredb/types.h>
#include <xfiredb/os.h>
#include <xfiredb/mem.h>

void atomic_destroy(atomic_t *atom)
{
	xfire_spinlock_destroy(&atom->lock);
}

void atomic64_destroy(atomic64_t *atom)
{
	xfire_spinlock_destroy(&atom->lock);
}

void atomic_add(atomic_t *atom, s32 val)
{
	xfire_spin_lock(&atom->lock);
	atom->val += val;
	xfire_spin_unlock(&atom->lock);
}

void atomic_sub(atomic_t *atom, s32 val)
{
	xfire_spin_lock(&atom->lock);
	atom->val -= val;
	xfire_spin_unlock(&atom->lock);
}

s32 atomic_get(atomic_t *atom)
{
	s32 tmp;

	xfire_spin_lock(&atom->lock);
	tmp = atom->val;
	xfire_spin_unlock(&atom->lock);

	return tmp;
}

void atomic64_add(atomic64_t *atom, s64 val)
{
	xfire_spin_lock(&atom->lock);
	atom->val += val;
	xfire_spin_unlock(&atom->lock);
}

void atomic64_sub(atomic64_t *atom, s64 val)
{
	xfire_spin_lock(&atom->lock);
	atom->val -= val;
	xfire_spin_unlock(&atom->lock);
}

s64 atomic64_get(atomic64_t *atom)
{
	s64 tmp;

	xfire_spin_lock(&atom->lock);
	tmp = atom->val;
	xfire_spin_unlock(&atom->lock);

	return tmp;
}

