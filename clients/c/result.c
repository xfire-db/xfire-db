/*
 *  XFireDB result
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
#include <stdint.h>

#include <xfiredb/xfiredb.h>

/**
 * @addtogroup query
 * @{
 */

/**
 * @brief Allocate a new result.
 * @param num Number of results to be allocated.
 *
 * @return A \p NULL terminated array of results.
 */
struct xfiredb_result **xfiredb_result_alloc(size_t num)
{
	void **ary;
	int i;

	ary = xfiredb_zalloc(sizeof(*ary) * (num + 1));
	for(i = 0; i < num; i++)
		ary[i] = xfiredb_zalloc(sizeof(struct xfiredb_result));

	ary[num] = NULL;

	return (struct xfiredb_result**)ary;
}

/**
 * @brief Parse a serie of results.
 * @param rp Pointer to an array of results.
 */
void xfiredb_result_parse(struct xfiredb_result **rp)
{
	struct xfiredb_result *r;
	char *tmp;
	int i;
	ssize_t numeral;

	for(i = 0; rp[i]; i++) {
		r = rp[i];

		switch(r->data.ptr[0]) {
		case '+':
			r->type = XFIREDB_STRING;
			tmp = strdup(&r->data.ptr[1]);
			xfiredb_free(r->data.ptr);
			r->data.ptr = tmp;
			r->status = XFIREDB_RESULT_SUCCESS;
			break;

		case '&':
			r->type = XFIREDB_BOOL;
			numeral = strcmp("&true", r->data.ptr) ? 0 : 1;
			xfiredb_free(r->data.ptr);
			r->data.ptr = NULL;
			r->data.boolean = numeral;
			r->status = XFIREDB_RESULT_SUCCESS;
			break;

		case '%':
			r->type = XFIREDB_FIXNUM;
			tmp = &r->data.ptr[1];
			numeral = atoi(tmp);
			xfiredb_free(r->data.ptr);
			r->data.si = numeral;
			r->status = XFIREDB_RESULT_SUCCESS;
			break;

		case '-':
		default:
			r->type = XFIREDB_STATUS;

			if(r->data.ptr[0] == '-')
				tmp = &r->data.ptr[1];
			else
				tmp = r->data.ptr;

			if(!strcmp("OK", tmp)) {
				r->status = XFIREDB_RESULT_OK | XFIREDB_RESULT_SUCCESS;
				xfiredb_free(r->data.ptr);
				r->data.ptr = NULL;
			} else if(!strcmp("nil", tmp)) {
				r->status = XFIREDB_RESULT_NIL;
				xfiredb_free(r->data.ptr);
				r->data.ptr = NULL;
			} else {
				r->status = XFIREDB_RESULT_MSG;
			}

			break;
		}
	}
}

/**
 * @brief Free a set of results.
 * @param __p Pointer to an array of results.
 */
void xfiredb_result_free(struct xfiredb_result **__p)
{
	int i;
	void **p = (void**)__p;
	struct xfiredb_result *r;

	for(i = 0; p[i]; i++) {
		r = p[i];

		if(r->data.ptr && (r->status == XFIREDB_RESULT_MSG || r->type == XFIREDB_STRING))
			xfiredb_free(r->data.ptr);

		xfiredb_free(r);
	}

	xfiredb_free(p);
}

/** @} */

