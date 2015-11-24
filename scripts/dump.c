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
#include <unistd.h>
#include <getopt.h>
#include <wordexp.h>

#include <xfiredb/xfiredb.h>
#include <xfiredb/log.h>
#include <xfiredb/types.h>
#include <xfiredb/disk.h>

static struct option long_opts[] = {
	{"dbfile",  required_argument, 0, 'd'},
	{"output",  required_argument, 0, 'o'},
	{"help",    no_argument, 0, 'h'},
};

static void usage(const char *prog)
{
	printf("Usage: %s -d <path to database>\n", prog);
}

static void help(const char *prog)
{
	usage(prog);
	printf("Dump a XFireDB disk file to STDOUT or any given file.\n" \
		"\n" \
		"Arguments are all mandatory.\n" \
		"  -h, --help                  Display this message.\n" \
		"  -d, --dbfile  <path>        Path to a database file.\n" \
		"  -o, --output <path>         Output file. If no output file is given, stdout will be used.\n");
}

static struct disk *disk_create_helper(const char *path)
{
	return disk_create(path);
}

static void disk_destroy_helper(struct disk *d)
{
	disk_destroy(d);
}

static const char *path_helper(const char *path)
{
	wordexp_t p;
	char **w;

	wordexp(path, &p, 0);
	w = p.we_wordv;

	if(p.we_wordc > 1)
		return w[1];
	else
		return w[0];
}

int main(int argc, char **argv)
{
	const char *dbfile = NULL;
	const char *output = NULL;
	FILE *ofile;
	struct disk *db;
	int option = 0, opt_idx = 0;

	while(true) {
		option = getopt_long(argc, argv, "o:d:h", long_opts, &opt_idx);
		if(option == -1)
			break;

		switch(option) {
		case 'o':
			output = optarg;
			break;
		case 'd':
			dbfile = optarg;
			break;
		case 'h':
			help(argv[0]);
			exit(EXIT_SUCCESS);
			break;
		default:
			usage(argv[0]);
			exit(EXIT_FAILURE);
			break;
		}
	}

	if(!dbfile) {
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	if(!output) {
		ofile = stdout;
	} else {
		output = path_helper(output);
		ofile = fopen(output, "w+");
	}

	xfiredb_log_init(NULL, NULL);
	dbfile = path_helper(dbfile);
	db = disk_create_helper(dbfile);

	disk_dump(db, ofile);

	if(output)
		fclose(ofile);
	disk_destroy_helper(db);
	xfiredb_log_exit();
	return -EXIT_SUCCESS;
}

