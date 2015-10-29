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

#ifndef __LOG__H__
#define __LOG__H__

#include <xfiredb/engine/xfiredb.h>

#define XFIRE_ENGINE_LOG "ENGINE"
#define XFIRE_STORAGE_LOG "STORAGE"

#define LOG_INIT "init"
#define LOG_DISK "disk"

CDECL
extern void xfire_log_init(const char *out, const char *err);
extern void xfire_log_exit(void);
extern void xfire_log(const char *src, const char *msg, ...);
extern void xfire_log_err(const char *src, const char *msg, ...);
extern void xfire_log_console(const char *src, const char *fmt, ...);
CDECL_END

#endif

