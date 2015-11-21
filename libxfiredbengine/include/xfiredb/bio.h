/*
 *  Background I/O header
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

/**
 * @addtogroup bio
 * @{
 */

#ifndef __BIO_H__
#define __BIO_H__

#include <xfiredb/xfiredb.h>
#include <xfiredb/types.h>
#include <xfiredb/dict.h>
#include <xfiredb/os.h>
#include <xfiredb/database.h>

struct bio_q;

struct bio_q_head {
	struct bio_q *next,
		     *tail;
	struct job *job;

	xfire_spinlock_t lock;
	long size;
};

/**
 * @brief BIO operation type.
 */
typedef enum {
	STRING_ADD, //!< Add a string.
	STRING_DEL, //!< Delete a string.
	STRING_UPDATE, //!< Update a string.

	LIST_ADD, //!< Add a list (entry).
	LIST_DEL, //!< Delete a list (entry).
	LIST_UPDATE, //!< Update a list (entry).

	HM_ADD, //!< Add a hashmap entry.
	HM_DEL, //!< Delete a hashmap entry.
	HM_UPDATE, //!< Update a hashmap entry.

	SET_ADD, //!<  Add a set key.
	SET_DEL, //!< Delete a set key.
} bio_operation_t;

/**
 * @brief Back ground I/O queue.
 */
struct bio_q {
	struct bio_q *next, //!< Next pointer.
		     *prev; //!< Previous pointer.

	char *key; //!< Entry key.
	/**
	 * @brief Database argument.
	 *
	 * The contents of \p arg vary, depending on which
	 * kind of entry is being altered or added. In case
	 * of lists, the \p arg points to the data that is
	 * currently in the database. In case of hashmaps, it
	 * contains the key within the hashmap.
	 */
	char *arg;
	char *newdata; //!< New data, in case we are adding or updating.

	bio_operation_t operation; //!< Type of operation.
};

CDECL
extern void bio_init(void);
extern void bio_exit(void);
extern void bio_queue_add(char *key, char *arg,
	       		char *newdata, bio_operation_t op);
extern void dbg_bio_queue(void);
extern void bio_sync(void);
CDECL_END

#endif

/** @} */

