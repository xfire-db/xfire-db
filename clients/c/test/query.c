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

#include <config.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <xfiredb/xfiredb.h>

static void numeral_test(struct xfiredb_client *client)
{
	struct xfiredb_result **r;
	int i;
	char *query_add = "SADD setkey \"key1\" \"key2\" \"key3\" \"key4\"";
	char *query_del = "DELETE setkey";

	printf("Executing numeral test.\nAdding 4 keys to a set:\n");
	r = xfiredb_query(client, query_add);
	for(i = 0; r[i]; i++) {
		if(xfiredb_result_type(r[i]) == XFIREDB_FIXNUM) {
#ifdef HAVE_X64
			printf("Number of set keys inserted: %li\n", (long)xfiredb_result_to_int(r[i]));
#else
			printf("Number of set keys inserted: %i\n", xfiredb_result_to_int(r[i]));
#endif
		} else {
			printf("An error occurred");
			if(xfiredb_result_type(r[i]) == XFIREDB_STATUS && xfiredb_result_status(r[i]) == XFIREDB_RESULT_MSG)
				printf(": %s\n", xfiredb_result_to_ptr(r[i]));
		}
	}
	xfiredb_result_free(r);

	printf("\nSet test done, deleting set:");
	r = xfiredb_query(client, query_del);
	if(r[0]->type == XFIREDB_STATUS && (r[0]->status & XFIREDB_RESULT_OK))
		printf("OK\n");
	else
		printf("ERROR!\n");
	xfiredb_result_free(r);
}

#define HASH_QUERY "MADD hash-key %s \"%s\""
static void hash_test(struct xfiredb_client *client)
{
	int i;
	char *data1 = "Data for \"key1\"";
	char *data2 = "Data for \"key2\"";
	char *data3 = "Data for \"key3\"";
	char *query, *tmp;
	struct xfiredb_result **r;

	tmp = xfiredb_escape_string(data1);
	xfiredb_sprintf(&query, HASH_QUERY, "hkey1", tmp);
	r = xfiredb_query(client, query);

	if(!xfiredb_result_success(r[0])) {
		printf("Query error!\n");
		abort();
	}
	xfiredb_result_free(r);
	xfiredb_escape_free(tmp);
	xfiredb_free(query);

	tmp = xfiredb_escape_string(data2);
	xfiredb_sprintf(&query, HASH_QUERY, "hkey2", tmp);
	r = xfiredb_query(client, query);

	if(!xfiredb_result_success(r[0])) {
		printf("Query error!\n");
		abort();
	}
	xfiredb_result_free(r);
	xfiredb_escape_free(tmp);
	xfiredb_free(query);

	tmp = xfiredb_escape_string(data3);
	xfiredb_sprintf(&query, HASH_QUERY, "hkey3", tmp);
	r = xfiredb_query(client, query);

	if(!xfiredb_result_success(r[0])) {
		printf("Query error!\n");
		abort();
	}
	xfiredb_result_free(r);
	xfiredb_escape_free(tmp);
	xfiredb_free(query);

	/* GET data */
	query = "MREF hash-key hkey1 hkey2 hkey3";
	r = xfiredb_query(client, query);

	for(i = 0; r[i]; i++) {
		if(!xfiredb_result_success(r[i])) {
			printf("Query error!\n");
			abort();
		}

		tmp = xfiredb_unescape_string(xfiredb_result_to_ptr(r[i]));
		printf("%s\n", tmp);
		xfiredb_escape_free(tmp);
	}
	xfiredb_result_free(r);

	r = xfiredb_query(client, "DELETE hash-key");
	if(!xfiredb_result_success(r[0])) {
		printf("Query error!\n");
		abort();
	}
	xfiredb_result_free(r);
}

int main(int argc, char **argv)
{
	struct xfiredb_client *client;
	struct xfiredb_result **result;
	char *ip, *_port;
	int port, i;

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
	numeral_test(client);
	printf("\n");
	hash_test(client);
	printf("\n");
	result = xfiredb_query(client, "lref tl 0..-1");
	xfiredb_disconnect(client);

	for(i = 0; result[i]; i++)
		printf("%s\n", result[i]->data.ptr);

	xfiredb_result_free(result);
	return -EXIT_SUCCESS;
}

