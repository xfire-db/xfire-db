/*
 *  XFireDB query unit test
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
#include <string.h>

#include <xfiredb/xfiredb.h>

static int dbg_query(struct xfiredb_client *client, const char *query)
{
	char tmp[1024];
	char *querynl;
	int len;

	len = strlen(query);
	querynl = xfire_zalloc(len + 2);
	memcpy(querynl, query, len);
	querynl[len] = '\n';
	querynl[len + 1] = '\0';
	len = strlen(querynl);

	write(client->socket, querynl, len);
	xfire_free(querynl);
	len = read(client->socket, tmp, sizeof(tmp));
	tmp[len-1] = '\0';
	printf("Received: %s\n", tmp);
	return 0;
}
static int dbg_ssl_query(struct xfiredb_client *client, const char *query)
{
	char tmp[1024];
	char *querynl;
	int len;

	len = strlen(query);
	querynl = xfire_zalloc(len + 2);
	memcpy(querynl, query, len);
	querynl[len] = '\n';
	querynl[len + 1] = '\0';
	len = strlen(querynl);

	SSL_write(client->ssl->ssl, querynl, len);
	xfire_free(querynl);
	len = SSL_read(client->ssl->ssl, tmp, sizeof(tmp));
	tmp[len-1] = '\0';
	printf("Received: %s\n", tmp);
	return 0;
}

int main(int argc, char **argv)
{
	struct xfiredb_client *client;
	char *ip, *_port;
	int port;

	if(argc != 3) {
		fprintf(stderr, "Usage: %s <hostname> <portnum>\n", argv[0]);
		exit(1);
	} else {
		ip = argv[1];
		_port = argv[2];
	}

	port = atoi(_port);
	client = xfiredb_connect(ip, port, XFIREDB_SSL | XFIREDB_AUTH | XFIREDB_SOCK_STREAM);

	if(!client) {
		fprintf(stderr, "Could not connect to server!\n");
		exit(1);
	}

	xfiredb_auth_client(client, "cluster", "cluster");
	dbg_query(client, "GET key1");
	xfiredb_disconnect(client);

	return -EXIT_SUCCESS;
}

