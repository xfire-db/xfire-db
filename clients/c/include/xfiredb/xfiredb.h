/*
 *  XFireDB client header
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

#ifndef __XFIREDB_H__
#define __XFIREDB_H__

#include <openssl/ssl.h>

#define XFIRE_OK 0 //!< XFireDB success error code
#define XFIRE_ERR 1 //!< XFireDB general error code
#define XFIRE_NOTCONN 2 //!< Not connected error
#define XFIRE_AUTH 3 //!< Not authorized error

#ifdef __cplusplus
#define CDECL extern "C" {
#define CDECL_END }
#else
#define CDECL
#define CDECL_END
#endif

struct xfiredb_ssl_client {
	SSL_CTX *ctx;
	SSL *ssl;
};

struct xfiredb_client {
	int socket;
	struct sockaddr_in *serv;
	struct xfiredb_ssl_client *ssl;

	long flags;
};

typedef enum {
	XFIREDB_STRING,
	XFIREDB_FIXNUM,
	XFIREDB_STATUS,
} xfiredb_result_type_t;

struct xfiredb_result {
	const char *data;
	int length;

	xfiredb_result_type_t type;
};

#define XFIREDB_SOCK_STREAM	0x1
#define XFIREDB_SOCK_RESOLV	0x2
#define XFIREDB_SSL		0x4
#define XFIREDB_AUTH		0x8

CDECL
extern struct xfiredb_client *xfiredb_connect(char *host, int port, long flags);
extern void xfiredb_disconnect(struct xfiredb_client *);
extern int xfiredb_auth_client(struct xfiredb_client *client, char *username, char *password);

extern struct xfiredb_result *xfiredb_query(struct xfiredb_client *client, const char *query);

extern void xfiredb_result_free(void *p);
extern struct xfiredb_result *xfiredb_result_alloc(size_t num);

extern int xfiredb_sprintf(char **buf, const char *format, ...);
extern char** str_split(char* a_str, const char a_delim);

extern void *xfire_alloc(size_t len);
extern void *xfire_zalloc(size_t len);
extern void *xfire_calloc(size_t num, size_t size);
extern void xfire_free(void *region);
extern void *xfire_realloc(void *region, size_t size);
CDECL_END

#endif
