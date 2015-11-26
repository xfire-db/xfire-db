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

#include <xfiredb/xfiredb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

static char *query_readline(struct xfiredb_client *client)
{
	char *tmp, *rv;
	int i, len;
	int ssl = !!(client->flags & XFIREDB_SSL);
	int num;
	char c;

	tmp = rv = xfire_zalloc(1024);

again:
	for(i = 0; i < 1022; i++) {
		if(ssl)
			num = SSL_read(client->ssl->ssl, &c, 1);
		else
			num = read(client->socket, &c, 1);
		if(num <= 0)
			goto done;

		tmp[i] = c;
		if(c == '\n') {
			i++;
			goto done;
		}
	}

	tmp[i+1] = '\0';
	if(tmp != rv) {
		len = strlen(tmp) + strlen(rv) + 1;
		rv = realloc(rv, len);
		strcat(rv, tmp);
	}

	tmp = xfire_zalloc(1024);
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

static struct xfiredb_result *__query(struct xfiredb_client *client, const char *query)
{
	return NULL;
}

static struct xfiredb_result *ssl_query(struct xfiredb_client *client, const char *query)
{
	struct xfiredb_result *res;
	int len;
	ssize_t num;
	char *query2;

	if(!client || !query)
		return NULL;

	xfiredb_sprintf(&query2, "%s\n", query);
	len = strlen(query2);
	num = SSL_write(client->ssl->ssl, query2, len);
	xfire_free(query2);

	if(num <= 0L)
		return NULL;

	return NULL;
}

struct xfiredb_result *xfiredb_query(struct xfiredb_client *client, const char *query)
{
	if(!client)
		return NULL;

	if(client->flags & XFIREDB_SSL)
		return ssl_query(client, query);
	else
		return __query(client, query);
}

