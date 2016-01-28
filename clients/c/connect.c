/*
 *  XFireDB connect
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
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <xfiredb/xfiredb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

static int initialised = 0;

/**
 * @brief Initialise the XFireDB client library.
 */
void xfiredb_init(void)
{
	SSL_library_init();
	OpenSSL_add_all_algorithms();
	SSL_load_error_strings();

	initialised = 1;
}

static SSL_CTX *xfiredb_ssl_init(void)
{
	const SSL_METHOD *method;
	SSL_CTX *ctx;

	method = SSLv23_client_method();
	ctx = SSL_CTX_new(method);

	if(!ctx) {
		ERR_print_errors_fp(stderr);
		abort();
	}

	return ctx;
}

static int hostname_to_ip(const char *host, char *ip)
{
	struct hostent *he;
	struct in_addr **addr_list;
	int i = 0;

	he = gethostbyname(host);
	if(!he)
		return -XFIREDB_ERR;

	addr_list = (struct in_addr**) he->h_addr_list;
	for(; addr_list[i]; i++) {
		strcpy(ip, inet_ntoa(*addr_list[i]));
		return -XFIREDB_OK;
	}

	return -XFIREDB_OK;
}

static struct xfiredb_client *xfiredb_ssl_connect(const char *hostname, int port, long flags)
{
	struct xfiredb_client *client;
	struct sockaddr_in addr;
	struct hostent *host;
	int sock;

	if(!initialised)
		xfiredb_init();

	client = xfiredb_zalloc(sizeof(*client));
	client->ssl = xfiredb_zalloc(sizeof(*client->ssl));
	client->ssl->ctx = xfiredb_ssl_init();

	if((host = gethostbyname(hostname)) == NULL) {
		xfiredb_free(client->ssl);
		xfiredb_free(client);
		return NULL;
	}

	sock = socket(AF_INET, SOCK_STREAM, 0);
	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = *(long*)(host->h_addr);

	if(connect(sock, (struct sockaddr*)&addr, sizeof(addr))) {
		close(sock);
		xfiredb_free(client->ssl);
		xfiredb_free(client);
		return NULL;
	}

	client->ssl->ssl = SSL_new(client->ssl->ctx);
	SSL_set_fd(client->ssl->ssl, sock);
	if(SSL_connect(client->ssl->ssl) == -1) {
		fprintf(stderr, "xfiredb: cound not connect to SSL server!\n");
		close(sock);
		SSL_CTX_free(client->ssl->ctx);
		xfiredb_free(client->ssl);
		xfiredb_free(client);
		return NULL;
	}

	client->flags = flags;
	client->socket = sock;

	return client;
}

static struct xfiredb_client *__xfiredb_connect(const char *hostname, int port, long flags)
{
	int sock;
	struct sockaddr_in addr;
	struct hostent *serv;
	struct xfiredb_client *client;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0)
		return NULL;

	serv = gethostbyname(hostname);
	if(!serv) {
		close(sock);
		return NULL;
	}

	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = *(long*)(serv->h_addr);
	addr.sin_port = htons(port);

	/* connect to the server */
	if(connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		close(sock);
		printf("Details: %s :: %li :: %i\n", hostname, *(long*)serv->h_addr, port);
		return NULL;
	}

	client = xfiredb_zalloc(sizeof(*client));
	client->socket = sock;
	client->flags = flags;

	return client;
}

#define XFIREDB_STREAM_COMMAND "STREAM\n"
#define SIZE_OF_BUF(__b) (sizeof(__b) - 1)

/**
 * @brief Connect to a XFireDB server.
 *
 * @param host Server host.
 * @param port Server port.
 * @param flags Connection settings.
 * @return The connected client or \p NULL if an error occurred.
 */
struct xfiredb_client *xfiredb_connect(const char *host, int port, long flags)
{
	struct xfiredb_client *client;
	char tmp[16];

	if((flags & XFIREDB_SOCK_RESOLV) != 0L) {
		if(hostname_to_ip(host, tmp) != -XFIREDB_OK)
			return NULL;
		else
			host = tmp;
	}

	if((flags & XFIREDB_SSL) != 0L)
		client = xfiredb_ssl_connect(host, port, flags);
	else
		client = __xfiredb_connect(host, port, flags);

	if(!client)
		return NULL;

	if((flags & (XFIREDB_SOCK_STREAM | XFIREDB_AUTH)) == XFIREDB_SOCK_STREAM) {
		if(flags & XFIREDB_SSL)
			SSL_write(client->ssl->ssl, XFIREDB_STREAM_COMMAND, SIZE_OF_BUF(XFIREDB_STREAM_COMMAND));
		else
			write(client->socket, XFIREDB_STREAM_COMMAND, SIZE_OF_BUF(XFIREDB_STREAM_COMMAND));
	}

	return client;
}

/**
 * @brief Authenticate a connected client.
 *
 * @param client Client structure.
 * @param username Username to authenticate as.
 * @param password Password for \p username.
 * @return An error code.
 */
int xfiredb_auth_client(struct xfiredb_client *client, const char *username, const char *password)
{
	char *query;
	char tmp[1024];
	int num = 0;

	xfiredb_sprintf(&query, "AUTH %s %s\n", username, password);

	if(client->flags & XFIREDB_SSL) {
		SSL_write(client->ssl->ssl, query, strlen(query));
		num = SSL_read(client->ssl->ssl, tmp, sizeof(tmp));
		tmp[num] = '\0';

		if(strcmp(tmp, "OK\n")) {
			xfiredb_free(query);
			return -XFIREDB_ERR;
		}

		if(client->flags & XFIREDB_SOCK_STREAM)
			SSL_write(client->ssl->ssl, XFIREDB_STREAM_COMMAND, SIZE_OF_BUF(XFIREDB_STREAM_COMMAND));
	} else {
		write(client->socket, query, strlen(query));
		num = read(client->socket, tmp, sizeof(tmp));
		tmp[num] = '\0';

		if(strcmp(tmp, "OK\n")) {
			xfiredb_free(query);
			return -XFIREDB_ERR;
		}

		if(client->flags & XFIREDB_SOCK_STREAM)
			write(client->socket, XFIREDB_STREAM_COMMAND, SIZE_OF_BUF(XFIREDB_STREAM_COMMAND));
	}

	xfiredb_free(query);
	return -XFIREDB_OK;
}

/**
 * @brief Disconnect a connected client.
 *
 * @param client Client to disconnect.
 */
void xfiredb_disconnect(struct xfiredb_client *client)
{
	if(!client)
		return;

	if((client->flags & XFIREDB_SSL) && client->ssl) {
	       	SSL_free(client->ssl->ssl);
		SSL_CTX_free(client->ssl->ctx);
		xfiredb_free(client->ssl);
	}

	close(client->socket);
	if(client->serv)
		xfiredb_free(client->serv);

	xfiredb_free(client);
}

