/*
 *  Thread sleeping
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
#include <time.h>
#include <unistd.h>

#include <xfiredb/xfiredb.h>
#include <xfiredb/time.h>

#ifdef HAVE_POSIX
void xfiredb_sleep(int secs)
{
	sleep(secs);
}

void xfiredb_sleep_ms(int ms)
{
	xfiredb_sleep_ns((long)ms * 1000000);
}

void xfiredb_sleep_ns(long ns)
{
	long secs;
	struct timespec tspec;

	secs = ns / 1000000000L;

	if(secs)
		ns -= secs * 1000000000L;

	tspec.tv_sec = secs;
	tspec.tv_nsec = ns;
	nanosleep(&tspec, NULL);
}
#endif

