/*
 *  LOG library
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
 * @addtogroup log
 * @{
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include <xfiredb/xfiredb.h>
#include <xfiredb/types.h>
#include <xfiredb/log.h>

static FILE *xfiredb_stdout = NULL;
static FILE *xfiredb_stderr = NULL;

/**
 * @brief Initialise XFire logging.
 * @param out stdout file name.
 * @param err stderr file name.
 */
void xfiredb_log_init(const char *out, const char *err)
{
	if(out)
		xfiredb_stderr = fopen(err, "w+");
	else
		xfiredb_stderr = stderr;

	if(err)
		xfiredb_stdout = fopen(out, "w+");
	else
		xfiredb_stdout = stdout;
}

/**
 * @brief Close the logging streams.
 */
void xfiredb_log_exit(void)
{
	raw_xfiredb_log("[exit]: XFIRE logger stopped.\n");
	fclose(xfiredb_stderr);
	fclose(xfiredb_stdout);

}

static void vfxfiredb_log(const char *src, const char *fmt, va_list args)
{
	fprintf(xfiredb_stdout, "[%s]: ", src);
	vfprintf(xfiredb_stdout, fmt, args);
	fflush(xfiredb_stdout);
}

/**
 * @brief Log a message during the initialisation.
 * @param src Logging source.
 * @param fmt Log format.
 * @param ... Variable argument list.
 */
void xfiredb_log_console(const char *src, const char *fmt, ...)
{
	va_list args, args2;

	va_start(args, fmt);
	va_copy(args2, args);
	vfxfiredb_log(src, fmt, args);
	va_end(args);

	if(xfiredb_stdout != stdout) {
		fprintf(stdout, "[%s]: ", src);
		vfprintf(stdout, fmt, args2);
	}

	va_end(args2);
}

/**
 * @brief Raw error logger.
 * @param msg Message to log.
 */
void raw_xfiredb_log_err(const char *msg)
{
	fprintf(xfiredb_stderr, msg);
	fflush(xfiredb_stderr);
}

/**
 * @brief Raw logger.
 * @param msg Message to log.
 */
void raw_xfiredb_log(const char *msg)
{
	fprintf(xfiredb_stdout, msg);
	fflush(xfiredb_stdout);
}

void raw_xfiredb_log_console(const char *msg)
{
	if(xfiredb_stdout != stdout)
		fprintf(stdout, msg);
	raw_xfiredb_log(msg);
}

/**
 * @brief Log a message.
 * @param src Source of the log (i.e. the subsystem.
 * @param msg Format string.
 * @param ... Variable arguments to match \p msg.
 */
void xfiredb_log(const char *src, const char *msg, ...)
{
	va_list args;

	va_start(args, msg);
	vfxfiredb_log(src, msg, args);
	va_end(args);
}

/**
 * @brief Log a message.
 * @param src Source of the log (i.e. the subsystem.
 * @param msg Format string.
 * @param ... Variable arguments to match \p msg.
 */
void xfiredb_log_err(const char *src, const char *msg, ...)
{
	va_list args;

	va_start(args, msg);
	fprintf(xfiredb_stderr, "[%s]: ", src);
	vfprintf(xfiredb_stderr, msg, args);
	va_end(args);
}

/** @} */

