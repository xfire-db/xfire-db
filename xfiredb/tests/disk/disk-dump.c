/*
 *  Disk dumper
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

#include <sys/time.h>

#include <xfiredb/engine/xfiredb.h>
#include <xfiredb/engine/types.h>
#include <xfiredb/engine/mem.h>
#include <xfiredb/engine/disk.h>
#include <xfiredb/engine/bio.h>
#include <xfiredb/engine/string.h>

int main(int argc, char **argv)
{
	xfiredb_init();
	disk_dump(disk_db);
	xfiredb_exit();
	return -EXIT_SUCCESS;
}
