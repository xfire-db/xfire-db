/*
 *  XFireDB query
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

#ifdef __unix__
#include <sys/socket.h>
#include <sys/types.h>
#else
#include <winsock2.h>
#endif

#include <xfiredb/xfiredb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

/**
 * @addtogroup query
 * @{
 */

static char *query_readline(struct xfiredb_client *client)
{
	char *tmp, *rv;
	int i, len;
	int ssl = !!(client->flags & XFIREDB_SSL);
	int num;
	char c;

	tmp = rv = xfiredb_zalloc(1024);

again:
	for(i = 0; i < 1022; i++) {
		if(ssl)
			num = SSL_read(client->ssl->ssl, &c, 1);
		else
			num = recv(client->socket, &c, 1, 0);

		if(num <= 0 || c == '\n')
			goto done;

		tmp[i] = c;
	}

	tmp[i] = '\0';
	if(tmp != rv) {
		len = strlen(tmp) + strlen(rv) + 1;
		rv = realloc(rv, len);
		strcat(rv, tmp);
	}

	tmp = xfiredb_zalloc(1024);
	goto again;

done:
	tmp[i] = '\0';

	if(tmp != rv) {
		len = strlen(rv) + strlen(tmp) + 1;
		rv = realloc(rv, len);
		strcat(rv, tmp);
	} else {
		rv = realloc(rv, strlen(rv) + 1);
	}

	return rv;
}

static struct xfiredb_result **__query(struct xfiredb_client *client, const char *query)
{
	struct xfiredb_result **res;
	int len, i;
	ssize_t num, size;
	char *query2, *buf, **sizes;

	if(!query)
		return NULL;

	xfiredb_sprintf(&query2, "%s\n", query);
	len = strlen(query2);
	num = send(client->socket, query2, len, 0);
	xfiredb_free(query2);

	if(num <= 0L)
		return NULL;

	buf = query_readline(client);
	len = str_count_occurences(buf, ' ') + 1;
	sizes = str_split(buf, ' ');
	res = xfiredb_result_alloc(len);
	xfiredb_free(buf);

	for(i = 0; i < len; i++) {
		size = atoi(sizes[i]);
		res[i]->data.ptr = xfiredb_zalloc(size);
		num = recv(client->socket, res[i]->data.ptr, size, 0);
		res[i]->data.ptr[num-1] = '\0';
		xfiredb_free(sizes[i]);
	}

	xfiredb_free(sizes);

	return res;
}

static struct xfiredb_result **ssl_query(struct xfiredb_client *client, const char *query)
{
	struct xfiredb_result **res;
	struct xfiredb_ssl_client *ssl;
	int len, i;
	ssize_t num, size;
	char *query2, *buf, **sizes;

	if(!client->ssl || !query)
		return NULL;

	ssl = client->ssl;
	xfiredb_sprintf(&query2, "%s\n", query);
	len = strlen(query2);
	num = SSL_write(ssl->ssl, query2, len);
	xfiredb_free(query2);

	if(num <= 0L)
		return NULL;

	buf = query_readline(client);
	len = str_count_occurences(buf, ' ') + 1;
	sizes = str_split(buf, ' ');
	res = xfiredb_result_alloc(len);
	xfiredb_free(buf);

	for(i = 0; i < len; i++) {
		size = atoi(sizes[i]);
		res[i]->data.ptr = xfiredb_zalloc(size);
		num = SSL_read(ssl->ssl, res[i]->data.ptr, size);
		res[i]->data.ptr[num-1] = '\0';
		xfiredb_free(sizes[i]);
	}

	xfiredb_free(sizes);

	return res;
}

/**
 * @brief Query a XFireDB server.
 * @param client XFireDB client object.
 * @param query XQL query to sent to the server.
 * @return The query result.
 */
struct xfiredb_result **xfiredb_query(struct xfiredb_client *client, const char *query)
{
	struct xfiredb_result **rv;

	if(!client)
		return NULL;

	if(client->flags & XFIREDB_SSL)
		rv = ssl_query(client, query);
	else
		rv = __query(client, query);

	if(!rv)
		return NULL;

	xfiredb_result_parse(rv);
	return rv;
}

/** @} */

