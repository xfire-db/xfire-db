/*
 *  Background processes
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
#include <unistd.h>

#include <xfiredb/xfiredb.h>
#include <xfiredb/types.h>
#include <xfiredb/bio.h>
#include <xfiredb/bg.h>
#include <xfiredb/mem.h>
#include <xfiredb/error.h>
#include <xfiredb/disk.h>

extern struct disk *dbg_disk;

int main(int argc, char **argv)
{
	xfiredb_init();
	dbg_bio_queue();
	sleep(1);
	bg_process_signal("bio-worker");
	sleep(1);
	disk_dump(dbg_disk, stdout);
	
	xfiredb_exit();

	return -EXIT_SUCCESS;
}

