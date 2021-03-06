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

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include <xfiredb/xfiredb.h>
#include <xfiredb/types.h>
#include <xfiredb/bg.h>
#include <xfiredb/bio.h>
#include <xfiredb/database.h>
#include <xfiredb/mem.h>
#include <xfiredb/os.h>
#include <xfiredb/error.h>
#include <xfiredb/disk.h>
#include <xfiredb/time.h>

extern struct disk *dbg_disk;
#ifndef HAVE_DEBUG
extern struct disk *xfiredb_disk;
#endif
static struct bio_q_head *bio_q;

#define BIO_WORKER_NAME "bio-worker"

static inline struct bio_q *bio_queue_pop(void)
{
	struct bio_q *tail;

	xfiredb_spin_lock(&bio_q->lock);
	tail = bio_q->tail;
	
	if(tail) {
		bio_q->tail = tail->prev;

		if(tail->prev) {
			tail->prev->next = NULL;
		} else {
			bio_q->next = NULL;
		}

		tail->next = tail->prev = NULL;

		bio_q->size--;
	}
	xfiredb_spin_unlock(&bio_q->lock);

	return tail;
}

static int bio_try_wakeup_worker(void)
{
	static int wake_up_counter = 0;
	struct config *conf = xfiredb_get_config();

	wake_up_counter += 1;
	if((wake_up_counter % (conf->persist_level + 1)) == 0) {
		wake_up_counter = 0;
		bg_process_signal(BIO_WORKER_NAME);
		return 0;
	}

	return 1;
}

static void bio_worker(void *arg)
{
	struct disk *d = arg;
	struct bio_q *q;

	while((q = bio_queue_pop()) != NULL) {
		switch(q->operation) {
		case STRING_ADD:
			disk_store_string(d, q->key, q->newdata);
			break;
		case STRING_UPDATE:
			disk_update_string(d, q->key, q->newdata);
			break;
		case STRING_DEL:
			disk_delete_string(d, q->key);
			break;
		case LIST_ADD:
			disk_store_list_entry(d, q->key, q->newdata);
			break;
		case LIST_DEL:
			disk_delete_list(d, q->key, q->arg);
			break;
		case LIST_UPDATE:
			disk_update_list(d, q->key, q->arg, q->newdata);
			break;
		case HM_ADD:
			disk_store_hm_node(d, q->key, q->arg, q->newdata);
			break;
		case HM_DEL:
			disk_delete_hashmapnode(d, q->key, q->arg);
			break;
		case HM_UPDATE:
			disk_update_hm(d, q->key, q->arg, q->newdata);
			break;
		case SET_ADD:
			disk_store_set_key(d, q->key, q->arg);
			break;
		case SET_DEL:
			disk_delete_set_key(d, q->key, q->arg);
			break;
		default:
			break;
		}

		xfiredb_free(q->key);
		xfiredb_free(q->arg);
		xfiredb_free(q->newdata);
		xfiredb_free(q);
	}
}

/**
 * @brief Initialise the background I/O module.
 */
void bio_init(void)
{
	struct config *config;

	bio_q = xfiredb_zalloc(sizeof(*bio_q));
	bio_q->size = 0L;
	xfiredb_spinlock_init(&bio_q->lock);
	config = xfiredb_get_config();
	disk_db = disk_create(config->db_file);
	bio_q->job = bg_process_create(BIO_WORKER_NAME, &bio_worker, disk_db);
}

/**
 * @brief BIO destructor.
 */
void bio_exit(void)
{
	bg_process_stop(BIO_WORKER_NAME);
	xfiredb_spinlock_destroy(&bio_q->lock);
	xfiredb_free(bio_q);

	disk_destroy(disk_db);
}

static inline long bio_size(void)
{
	long s;

	xfiredb_spin_lock(&bio_q->lock);
	s = bio_q->size;
	xfiredb_spin_unlock(&bio_q->lock);

	return s;
}

/**
 * @brief Immediatly wake up the BIO worker.
 * @see xfiredb_sync
 */
void bio_sync(void)
{
	bg_process_signal(BIO_WORKER_NAME);

	while(bio_size())
		xfiredb_sleep_ns(100000);
}

/**
 * @brief Add an entry to the BIO queue.
 * @param key The key of the entry.
 * @param arg Extra info about the added entry. See below.
 * @param newdata In case of an add or update, the new data to set.
 * @param op Type of operation.
 *
 * The \p arg argument can either point to the currently stored data, in case
 * of a list operation, or to the key within the hashmap in case of a
 * hashmap operation.
 */
void bio_queue_add(char *key, char *arg, char *newdata, bio_operation_t op)
{
	struct bio_q *q;
	struct config *conf = xfiredb_get_config();

	if(conf->persist_level >= 3) {
		xfiredb_free(key);
		xfiredb_free(arg);
		xfiredb_free(newdata);
		return;
	}

	q = xfiredb_zalloc(sizeof(*q));
	q->key = key;
	q->arg = arg;
	q->newdata = newdata;
	q->operation = op;

	xfiredb_spin_lock(&bio_q->lock);
	bio_q->size++;
	if(bio_q->next) {
		q->next = bio_q->next;
		bio_q->next->prev = q;
		bio_q->next = q;
	} else {
		/* first entry */
		bio_q->next = bio_q->tail = q;
	}
	xfiredb_spin_unlock(&bio_q->lock);
	bio_try_wakeup_worker();
}

#if defined(HAVE_DEBUG) || defined(__DOXYGEN__)

static char *dbg_keys[] = {"fist-test", "second-test", "third-test"};
static char *dbg_data[] = {"first-data", "second-data", "second-data"};

static void dbg_add_strings(void)
{
	char *key, *data;
	int i, key_len, data_len;

	for(i = 0; i < 3; i++) {
		key_len = strlen(dbg_keys[i]);
		data_len = strlen(dbg_data[i]);
		key = xfiredb_zalloc(key_len + 1);
		data = xfiredb_zalloc(data_len + 1);
		memcpy(key, dbg_keys[i], key_len);
		memcpy(data, dbg_data[i], data_len);
		bio_queue_add(key, NULL, data, STRING_ADD);
	}
}

static void dbg_del_strings(void)
{
	char *key;
	int i, key_len;

	for(i = 0; i < 2; i++) {
		key_len = strlen(dbg_keys[i]);
		key = xfiredb_zalloc(key_len + 1);
		memcpy(key, dbg_keys[i], key_len);
		bio_queue_add(key, NULL, NULL, STRING_DEL);
	}
}

static void dbg_update_string(void)
{
	char *key, *data;
	int key_len, data_len;


	key_len = strlen("third-test");
	data_len = strlen("third-data");
	data = xfiredb_zalloc(data_len + 1);
	key = xfiredb_zalloc(key_len + 1);
	memcpy(data, "third-data", data_len);
	memcpy(key, "third-test", key_len);

	bio_queue_add(key, NULL, data, STRING_UPDATE);
}

static void dbg_add_list(void)
{
	char *key, *data;
	int i, key_len, data_len;


	key_len = strlen("list-key");

	for(i = 0; i < 3; i++) {
		data_len = strlen(dbg_data[i]);

		data = xfiredb_zalloc(data_len + 1);
		key = xfiredb_zalloc(key_len + 1);
		memcpy(data, dbg_data[i], data_len);
		memcpy(key, "list-key", key_len);

		bio_queue_add(key, NULL, data, LIST_ADD);
	}
}

static void dbg_update_list(void)
{
	char *key, *data, *oldata;
	int key_len, data_len;


	key_len = strlen("list-key");
	data_len = strlen("second-data");
	data = xfiredb_zalloc(data_len + 1);
	oldata = xfiredb_zalloc(data_len +1);
	key = xfiredb_zalloc(key_len + 1);
	memcpy(oldata, "second-data", data_len);
	memcpy(data, "SECOND-DATA", data_len);
	memcpy(key, "list-key", key_len);

	bio_queue_add(key, oldata, data, LIST_UPDATE);
}

static void dbg_del_list(void)
{
	char *key, *data;
	int key_len, data_len;

	key_len = strlen("list-key");
	data_len = strlen(dbg_data[1]);
	key = xfiredb_zalloc(key_len + 1);
	data = xfiredb_zalloc(data_len + 1);
	memcpy(data, dbg_data[1], data_len);
	memcpy(key, "list-key", key_len);
	bio_queue_add(key, data, NULL, LIST_DEL);
}

static char *dbg_snd_keys[] = {"key1", "key2", "key3" };
static void dbg_add_hashmap(void)
{
	char *key, *data, *subkey;
	int i, key_len, data_len, skey_len;

	key_len = strlen("hash-key");

	for(i = 0; i < 3; i++) {
		key = xfiredb_zalloc(key_len + 1);
		memcpy(key, "hash-key", key_len);
		data_len = strlen(dbg_data[i]);
		skey_len = strlen(dbg_snd_keys[i]);
		subkey = xfiredb_zalloc(skey_len + 1);
		data = xfiredb_zalloc(data_len + 1);
		memcpy(subkey, dbg_snd_keys[i], skey_len);
		memcpy(data, dbg_data[i], data_len);
		bio_queue_add(key, subkey, data, HM_ADD);
	}
}

static void dbg_update_hashmap(void)
{
	char *key, *skey, *ndata;
	int key_len, skey_len, ndata_len;

	key_len = strlen("hash-key");
	skey_len = strlen("key2");
	ndata_len = strlen("new-data");
	ndata = xfiredb_zalloc(ndata_len + 1);
	key = xfiredb_zalloc(key_len + 1);
	skey = xfiredb_zalloc(skey_len + 1);
	memcpy(ndata, "new-data", ndata_len);
	memcpy(key, "hash-key", key_len);
	memcpy(skey, "key2", skey_len);
	bio_queue_add(key, skey, ndata, HM_UPDATE);
}

static void dbg_del_hashmap(void)
{
	char *key, *skey;
	int key_len, skey_len;

	key_len = strlen("hash-key");
	skey_len = strlen("key3");
	key = xfiredb_zalloc(key_len + 1);
	skey = xfiredb_zalloc(skey_len + 1);
	memcpy(key, "hash-key", key_len);
	memcpy(skey, "key3", skey_len);
	bio_queue_add(key, skey, NULL, HM_DEL);
}

/**
 * @brief BIO debugging function.
 */
void dbg_bio_queue(void)
{
	dbg_add_hashmap();
	dbg_add_strings();
	dbg_add_list();
	dbg_del_strings();
	dbg_del_list();
	dbg_update_list();
	dbg_update_string();
	dbg_del_hashmap();
	dbg_update_hashmap();
}
#else
void dbg_bio_queue(void)
{
}
#endif

/** @} */

