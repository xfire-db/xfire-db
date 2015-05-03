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

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include <xfire/xfire.h>
#include <xfire/types.h>
#include <xfire/log.h>

#ifdef XFIRE_STDOUT
static FILE *xfire_stdout = NULL;
#define XFIRE_STDOUT xfire_stdout
#else
#define XFIRE_STDOUT stdout
#endif

#ifdef XFIRE_STDERR
static FILE *xfire_stderr = NULL;
#define XFIRE_STDERR xfire_stderr
#else
#define XFIRE_STDERR stderr
#endif

#ifdef XFIRE_LOG
void xfire_log_init(const char *out, const char *err)
{
	xfire_stderr = fopen(out, "r+a");
	xfire_stdout = fopen(err, "r+a");
}

void xfire_log_exit(void)
{
	fclose(xfire_stderr);
	fclose(xfire_stdout);

	fputs("XFIRE logger stopped.\n", stdout);
}
#endif

void xfire_log(const char *msg, ...)
{
	va_list args;

	va_start(args, msg);
	vfprintf(XFIRE_STDOUT, msg, args);
	va_end(args);
}

void xfire_log_err(const char *msg, ...)
{
	va_list args;

	va_start(args, msg);
	vfprintf(XFIRE_STDERR, msg, args);
	va_end(args);
}

