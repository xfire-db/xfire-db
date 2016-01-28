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
#include <string.h>
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
	{"host",  required_argument, 0, 'H'},
	{"port",  required_argument, 0, 'P'},
	{"auth",    no_argument, 0, 'a'},
	{"ssl",    no_argument, 0, 's'},
	{"help",    no_argument, 0, 'h'},
};

static void cli_usage(const char *prog)
{
	printf("Usage: %s -H <host> -P <port> -[upahs]\n", prog);
}

static void cli_help(const char *prog)
{
	cli_usage(prog);
	printf("Connect to an XFireDB server using a command line interface. \n" \
		"\n" \
		"   -H, --host <hostname>       Server address." \
		"   -P, --port <port>           Server port." \
		"   -a, --auth                  Read username/password from stdin." \
		"   -u, --user <username>       Username to use during authentication.\n" \
		"   -p, --pass <passowd>        Password to use during authentication.\n" \
		"   -s, --ssl                   Connect using SSL.\n" \
		"   -h, --help                  Display this help text.\n");
}

static struct xfiredb_client *cli_connect(const char *host, int port,
					const char *user, const char *pass, 
					int ssl, int auth)
{
	struct xfiredb_client *client;
	int flags = XFIREDB_SOCK_STREAM | XFIREDB_SOCK_RESOLV;

	if(auth)
		flags |= XFIREDB_AUTH;
	if(ssl)
		flags |= XFIREDB_SSL;

	if(!(client = xfiredb_connect(host, port, flags))) {
		printf("Failed to connect to server!\n");
		exit(EXIT_FAILURE);
	}

	if(auth) {
		if(xfiredb_auth_client(client, user, pass) != -XFIREDB_OK) {
			printf("Could not authenticate with the server!\n");
			xfiredb_disconnect(client);
			exit(EXIT_FAILURE);
		}
	}

	return client;
}

static void cli_run(struct xfiredb_client *client)
{
}

static void cli_getpass(char **user, char **pass)
{
	char buff[1024];

	printf("Username: ");
	fgets(buff, 1023, stdin);
	*user = xfiredb_zalloc(strlen(buff) + 1);
	strcpy(*user, buff);

	*pass = getpass("Password: ");
}

int main(int argc, char **argv)
{
	int option, opt_idx = 0, port = 0;
	int auth, ssl;
	char *user = NULL, *pass = NULL, *host = NULL;
	struct xfiredb_client *client = NULL;

	while(true) {
		option = getopt_long(argc, argv, "H:P:u:p:sha", long_opts, &opt_idx);
		if(option == -1)
			break;

		switch(option) {
		case 'a':
			cli_getpass(&user, &pass);
			auth = true;
			break;
		case 'p':
			pass = optarg;
			auth = true;
			break;
		case 'u':
			user = optarg;
			auth = true;
			break;
		case 'P':
			port = atoi(optarg);
			break;
		case 'H':
			host = optarg;
			break;
		case 's':
			ssl = true;
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

	if(!host || !port) {
		cli_usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	if(auth && (!pass || !user)) {
		printf("Both a username and password have to be supplied to " \
			"authenticate with a server.\n\n");
		cli_help(argv[0]);
		exit(EXIT_FAILURE);
	}

	client = cli_connect(host, port, user, pass, ssl, auth);
	cli_run(client);
	xfiredb_disconnect(client);

	return -EXIT_SUCCESS;
}
