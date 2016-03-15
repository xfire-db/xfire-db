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

#include <config.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifndef HAVE_WINDOWS
#include <termios.h>
#endif
#include <unistd.h>
#include <getopt.h>

#include <xfiredb/xfiredb.h>

#ifndef __cplusplus
#define false 0
#define true !false
#endif

#define COPYRIGHT_NOTICE \
  "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n" \
  "This is free software: you are free to change and redistribute it.\n" \
  "There is NO WARRANTY, to the extent permitted by law."

static struct option long_opts[] = {
	{"user",  required_argument, 0, 'u'},
	{"pass",  required_argument, 0, 'p'},
	{"host",  required_argument, 0, 'H'},
	{"port",  required_argument, 0, 'P'},
#ifndef HAVE_WINDOWS
	{"auth",    no_argument, 0, 'a'},
#endif
	{"ssl",    no_argument, 0, 's'},
	{"version", no_argument, 0, 'v'},
	{"help",    no_argument, 0, 'h'},
};

#ifndef HAVE_WINDOWS
static ssize_t __cli_getpass(char **lineptr, size_t *n, FILE *stream)
{
	struct termios old, new;
	int nread;

	/* Turn echoing off and fail if we can't. */
	if (tcgetattr (fileno (stream), &old) != 0)
	return -1;
	new = old;
	new.c_lflag &= ~ECHO;
	if(tcsetattr (fileno (stream), TCSAFLUSH, &new) != 0)
		return -1;

	/* Read the password. */
	nread = getline (lineptr, n, stream);

	/* Restore terminal. */
	tcsetattr (fileno (stream), TCSAFLUSH, &old);
	(*lineptr)[strlen(*lineptr)-1] = '\0';
	return nread;
}
#endif

static void cli_version(void)
{
	printf("XFireDB Command line interface\n%s\n", COPYRIGHT_NOTICE);
}

static void cli_usage(const char *prog)
{
	printf("Usage: %s -H <host> -P <port> -[upahs]\n", prog);
}

#ifndef HAVE_WINDOWS
#define AUTH_HELP "   -a, --auth                  Read username/password from stdin.\n"
#else
#define AUTH_HELP
#endif

static void cli_help(const char *prog)
{
	cli_usage(prog);
	printf("Connect to an XFireDB server using a command line interface. \n" \
		"\n" \
		"   -H, --host <hostname>       Server address.\n" \
		"   -P, --port <port>           Server port.\n" \
		AUTH_HELP \
		"   -u, --user <username>       Username to use during authentication.\n" \
		"   -p, --pass <password>       Password to use during authentication.\n" \
		"   -s, --ssl                   Connect using SSL.\n" \
		"   -v, --verion                Print version information.\n" \
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
		exit(-EXIT_FAILURE);
	}

	if(auth) {
		if(xfiredb_auth_client(client, user, pass) != -XFIREDB_OK) {
			printf("Could not authenticate with the server!\n");
			xfiredb_disconnect(client);
			exit(-EXIT_FAILURE);
		}
	}

	return client;
}

static void cli_run(struct xfiredb_client *client)
{
	char query[4096];
	struct xfiredb_result **r, *c;
	long status;
	unsigned char b;
	int i = 1;

	while(true) {
		memset(query, 0, 4096);
		printf("xfiredb> ");
		fgets(query, 4095, stdin);
		query[strlen(query)-1] = '\0';

		if(!strcmp(query, "quit"))
			break;

		r = xfiredb_query(client, query);

		if(!r) {
			printf("> Failed to query the server!\n");
			continue;
		}

		for(i = 0;; i++) {
			c = r[i];
			if(!c)
				break;

			switch(xfiredb_result_type(c)) {
			case XFIREDB_STRING:
				printf("> %s\n", xfiredb_result_to_ptr(c));
				break;

			case XFIREDB_BOOL:
				b = xfiredb_result_to_bool(c);
				printf("> (%s)\n", b ? "true" : "false");
				break;

			case XFIREDB_FIXNUM:
#ifndef HAVE_X64
				printf("> (%d)\n", xfiredb_result_to_int(c));
#else
				printf("> (%ld)\n", (long)xfiredb_result_to_int(c));
#endif
				break;

			case XFIREDB_STATUS:
				status = xfiredb_result_status(c);
				if(status & XFIREDB_RESULT_OK)
					printf("> (OK)\n");
				if(status & XFIREDB_RESULT_NIL)
					printf("> (nil)\n");
				if(status & XFIREDB_RESULT_MSG)
					printf("> %s\n", xfiredb_result_to_ptr(c));
				break;
			default:
				printf("> Failed to query the server!\n");
				break;
			}
		}

		xfiredb_result_free(r);
	}
}

#ifndef HAVE_WINDOWS
static void cli_getpass(char **user, char **pass)
{
	char buff[1024];
	size_t n = 0;

	printf("Username: ");
	fgets(buff, 1023, stdin);
	buff[strlen(buff)-1] = '\0';
	*user = xfiredb_zalloc(strlen(buff) + 1);
	strcpy(*user, buff);

	printf("Password: ");
	__cli_getpass(pass, &n, stdin);
	fputc('\n', stdout);
}
#endif

int main(int argc, char **argv)
{
	int option, opt_idx = 0, port = 0;
	int auth, ssl = false;
	char *user = NULL, *pass = NULL, *host = NULL;
	struct xfiredb_client *client = NULL;

	while(true) {
		option = getopt_long(argc, argv, "H:P:u:p:shva", long_opts, &opt_idx);
		if(option == -1)
			break;

		switch(option) {
#ifndef HAVE_WINDOWS
		case 'a':
			cli_getpass(&user, &pass);
			auth = true;
			break;
#endif
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
		case 'v':
			cli_version();
			exit(-EXIT_SUCCESS);
			break;
		case 'h':
			cli_help(argv[0]);
			exit(-EXIT_SUCCESS);
			break;
		default:
			cli_usage(argv[0]);
			exit(-EXIT_FAILURE);
			break;
		}
	}

	if(!host || !port) {
		cli_usage(argv[0]);
		exit(-EXIT_FAILURE);
	}

	if(auth && (!pass || !user)) {
		printf("Both a username and password have to be supplied to " \
			"authenticate with a server.\n\n");
		cli_usage(argv[0]);
		exit(-EXIT_FAILURE);
	}

	client = cli_connect(host, port, user, pass, ssl, auth);
	cli_run(client);
	xfiredb_disconnect(client);

	return -EXIT_SUCCESS;
}

