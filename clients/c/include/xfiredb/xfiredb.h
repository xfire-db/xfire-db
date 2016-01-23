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

/**
 * @brief SSL client data structure.
 */
struct xfiredb_ssl_client {
	SSL_CTX *ctx; //!< SSL CTX.
	SSL *ssl; //!< SSL handle.
};

/**
 * @brief XFireDB client data structure.
 */
struct xfiredb_client {
	int socket; //!< TCP socket.
	struct sockaddr_in *serv; //!< Socket address.
	struct xfiredb_ssl_client *ssl; //!< SSL client handle.

	long flags; //!< Connection flags.
};

/**
 * @brief Query results
 */
typedef enum {
	XFIREDB_STRING, //!< String result.
	XFIREDB_FIXNUM, //!< Numeral result.
	XFIREDB_STATUS, //!< Status code.
	XFIREDB_BOOL,   //!< Boolean result.
} xfiredb_result_type_t;

/**
 * @brief XFireDB query result data structure.
 */
struct xfiredb_result {
	/**
	 * @brief Result data.
	 */
	union {
		char *ptr; //!< Pointer data.
		size_t ui; //!< Unsigned numeral.
		ssize_t si; //!< Signed numeral.
		unsigned char boolean; //!< Boolean data.
	} data;

	int length; //!< Result length (size).

	xfiredb_result_type_t type; //!< Result type.
	long status; //!< Query status.
};

#define XFIREDB_RESULT_NIL 	0x1 //!< NIL result
#define XFIREDB_RESULT_OK	0x2 //!< Result code OK.
#define XFIREDB_RESULT_MSG	0x4 //!< Indicates that string data is returned.
#define XFIREDB_RESULT_SUCCESS	0x8 //!< Indicates the query was succesfull.

#define XFIREDB_SOCK_STREAM	0x1 //!< Do not terminate the socket after the first query.
#define XFIREDB_SOCK_RESOLV	0x2 //!< Hostname needs resolving.
#define XFIREDB_SSL		0x4 //!< Connect using SSL.
#define XFIREDB_AUTH		0x8 //!< Authenticate to the server using a username and password.

CDECL
/**
 * @brief Get the result type.
 * @param r Result to get the type from.
 * @return The result type of \p r.
 */
static inline xfiredb_result_type_t xfiredb_result_type(struct xfiredb_result *r)
{
	return r->type;
}

/**
 * @brief Get the result status.
 * @param r Result to get the status of.
 * @return The status of \p r.
 */
static inline long xfiredb_result_status(struct xfiredb_result *r)
{
	return r->status & (XFIREDB_RESULT_NIL | XFIREDB_RESULT_OK | XFIREDB_RESULT_MSG);
}

/**
 * @brief Convert the value of \p r to a signed integer.
 * @param r Result to get the integer data from.
 * @return Integer data stored in \p r.
 */
static inline ssize_t xfiredb_result_to_int(struct xfiredb_result *r)
{
	return r->data.si;
}

/**
 * @brief Get the unsigned integer value of \p r.
 * @param r Result to get the uint value of.
 * @return The integer data stored in \p r.
 */
static inline size_t xfiredb_result_to_uint(struct xfiredb_result *r)
{
	return r->data.ui;
}

/**
 * @brief Get the boolean value of \p r.
 * @param r Result to get the boolean value of.
 * @return The boolean data stored in \p r.
 */
static inline unsigned char xfiredb_result_to_bool(struct xfiredb_result *r)
{
	return r->data.boolean;
}

/**
 * @brief Get the data pointer stored in \p r.
 * @param r Result to get the data pointer from.
 * @return The data pointer stored in \p r.
 */
static inline char *xfiredb_result_to_ptr(struct xfiredb_result *r)
{
	return r->data.ptr;
}

/**
 * @brief Check if a query was executed succesfully.
 * @param r Result to check.
 * @return Non-zero if the query was succesful, zero if the query failed.
 */
static inline int xfiredb_result_success(struct xfiredb_result *r)
{
	return !!(r->status & XFIREDB_RESULT_SUCCESS);
}

extern struct xfiredb_client *xfiredb_connect(char *host, int port, long flags);
extern void xfiredb_disconnect(struct xfiredb_client *);
extern int xfiredb_auth_client(struct xfiredb_client *client, char *username, char *password);

extern struct xfiredb_result **xfiredb_query(struct xfiredb_client *client, const char *query);

extern void xfiredb_result_free(struct xfiredb_result **p);
extern struct xfiredb_result **xfiredb_result_alloc(size_t num);
extern void xfiredb_result_parse(struct xfiredb_result **rp);

extern char *xfiredb_escape_string(char *src);
extern char *xfiredb_unescape_string(char *src);
extern void xfiredb_escape_free(char *str);

extern int xfiredb_sprintf(char **buf, const char *format, ...);
extern char** str_split(char* a_str, const char a_delim);
extern int str_count_occurences(const char *str, char x);

extern void *xfire_alloc(size_t len);
extern void *xfire_zalloc(size_t len);
extern void *xfire_calloc(size_t num, size_t size);
extern void xfire_free(void *region);
extern void *xfire_realloc(void *region, size_t size);
CDECL_END

#endif

