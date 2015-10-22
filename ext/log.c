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

#include <xfire/xfire.h>
#include <xfire/types.h>
#include <xfire/log.h>

static FILE *xfire_stdout = NULL;
static FILE *xfire_stderr = NULL;

#if defined(HAVE_LOG) || defined(__DOXYGEN__)
/**
 * @brief Initialise XFire logging.
 * @param out stdout file name.
 * @param err stderr file name.
 */
void xfire_log_init(const char *out, const char *err)
{
#ifdef XFIRE_STDERR
	xfire_stderr = fopen(out, "r+a");
#else
	xfire_stderr = stderr;
#endif
#ifdef XFIRE_STDOUT
	xfire_stdout = fopen(err, "r+a");
#else
	xfire_stdout = stdout;
#endif
}

/**
 * @brief Close the logging streams.
 */
void xfire_log_exit(void)
{
#ifdef XFIRE_STDERR
	fclose(xfire_stderr);
#endif
#ifdef XFIRE_STDOUT
	fclose(xfire_stdout);
#endif

	fputs("XFIRE logger stopped.\n", stdout);
}
#endif

static void vfxfire_log(const char *src, const char *fmt, va_list args)
{
	fprintf(xfire_stdout, "[%s]: ", src);
	vfprintf(xfire_stdout, fmt, args);
}

/**
 * @brief Log a message during the initialisation.
 * @param src Logging source.
 * @param fmt Log format.
 * @param ... Variable argument list.
 */
void xfire_log_console(const char *src, const char *fmt, ...)
{
	va_list args, args2;

	va_start(args, fmt);
	va_copy(args2, args);
	vfxfire_log(src, fmt, args);
	va_end(args);

	fprintf(stdout, "[%s]: ", src);
	vfprintf(stdout, fmt, args2);
	va_end(args2);
}

/**
 * @brief Log a message.
 * @param src Source of the log (i.e. the subsystem.
 * @param msg Format string.
 * @param ... Variable arguments to match \p msg.
 */
void xfire_log(const char *src, const char *msg, ...)
{
	va_list args;

	va_start(args, msg);
	vfxfire_log(src, msg, args);
	va_end(args);
}

/**
 * @brief Log a message.
 * @param src Source of the log (i.e. the subsystem.
 * @param msg Format string.
 * @param ... Variable arguments to match \p msg.
 */
void xfire_log_err(const char *src, const char *msg, ...)
{
	va_list args;

	va_start(args, msg);
	fprintf(xfire_stderr, "[%s]: ", src);
	vfprintf(xfire_stderr, msg, args);
	va_end(args);
}

/** @} */

