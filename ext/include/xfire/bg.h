/*
 *  Background process header
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

#ifndef __BG_H__
#define __BG_H__

#include <stdlib.h>
#include <time.h>

#include <xfire/xfire.h>
#include <xfire/types.h>
#include <xfire/os.h>

struct job {
	char *name;
	time_t stamp;
	bool done;

	void (*handle)(void *arg);
	xfire_mutex_t lock;
	xfire_cond_t condi;
	struct thread *tp;
};

CDECL
extern void bg_processes_init(void);
extern void bg_processes_exit(void);

extern int bg_process_create(const char *name, void (*handle)(void *arg));
extern int bg_process_signal(const char *name);
extern int bg_process_stop(const char *name);
CDECL_END

#endif

