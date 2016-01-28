/*
 *  XFireDB CLI
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

#ifndef __cplusplus
#define false 0
#define true !false
#endif

static struct option long_opts[] = {
	{"user",  required_argument, 0, 'u'},
	{"pass",  required_argument, 0, 'p'},
	{"ssl",    no_argument, 0, 's'},
	{"help",    no_argument, 0, 'h'},
};

static void cli_usage(const char *prog)
{
	printf("Usage: %s [OPTS]\n", prog);
}

static void cli_help(const char *prog)
{
	cli_usage(prog);
	printf("Connect to an XFireDB server using a command line interface. \n" \
		"\n" \
		"   -u, --user <username>       Username to use during authentication.\n" \
		"   -p, --pass <passowd>        Password to use during authentication.\n" \
		"   -s, --ssl                   Connect using SSL.\n" \
		"   -h, --help                  Display this help text.\n");
}

int main(int argc, char **argv)
{
	int option, opt_idx = 0;

	while(true) {
		option = getopt_long(argc, argv, "u:p:sh", long_opts, &opt_idx);
		if(option == -1)
			break;

		switch(option) {
		case 'p':
			break;
		case 'u':
			break;
		case 's':
			break;
		case 'h':
			cli_help(argv[0]);
			exit(EXIT_SUCCESS);
			break;
		default:
			cli_usage(argv[0]);
			exit(EXIT_FAILURE);
			break;
		}
	}

	return -EXIT_SUCCESS;
}

