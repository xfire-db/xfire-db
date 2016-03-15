/*
 *  XFire client
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

#ifndef __XFIREDB_CLIENT_H_
#define __XFIREDB_CLIENT_H_

#include <config.h>
#include <xfiredb/compiler.h>
#ifndef __cplusplus
#include <xfiredb/types.h>
#endif

#ifdef __cplusplus
#define CDECL extern "C" {
#define CDECL_END }
#else
#define CDECL
#define CDECL_END
#endif

#ifndef __cplusplus
#ifndef true
#define true 1
#endif

#ifndef false
#define false !true
#endif

#endif

#define XOR(a,b) ((a) ^ (b))

struct disk;
struct database;
struct container;

extern struct disk *disk_db;
CDECL
extern struct config *xfiredb_get_config(void);
extern void xfiredb_se_init_silent(struct config *conf);
extern void xfiredb_se_init(struct config *conf);
extern void xfiredb_set_loadstate(bool v);
extern bool xfiredb_loadstate(void);
extern long xfiredb_disk_size(void);
extern void xfiredb_raw_load(void (*hook)(int argc, char **rows, char **cols));
extern void xfiredb_load_key(char *key, void (*hook)(int argc, char **rows, char **cols));
extern void xfiredb_se_exit(void);
extern void xfiredb_se_save(void);
extern void xfiredb_notice_disk(char *_key, char *_arg, char *_data, int op);
extern void xfiredb_store_container(char *_key, struct container *c);

extern int xfiredb_hashmap_clear(char *key, void (*hook)(char *key, char *data));
extern int xfiredb_list_clear(char *key, void (*hook)(char *key, char *data));
extern void xfiredb_disk_clear(void);
extern int xfiredb_sprintf(char **buf, const char *format, ...);
extern void xfiredb_init(void);
extern void xfiredb_exit(void);
extern int xfiredb_string_set(char *key, char *str);
extern int xfiredb_list_push(char *key, char *data, bool left);
extern int xfiredb_hashmap_set(char *key, char *skey, char *data);
extern int xfiredb_key_delete(char *key);
extern int xfiredb_string_get(char *key, char **data);
extern int xfiredb_list_get(char *key, char **data, int *idx, int num);
extern int xfiredb_list_pop(char *key, int *idx, int num);
extern int xfiredb_list_set(char *key, int idx, char *data);
extern int xfiredb_hashmap_get(char *key, char **skey, char **data, int num);
extern int xfiredb_hashmap_remove(char *key, char **skeys, int num);
extern int xfiredb_list_length(char *key);
CDECL_END

#endif

