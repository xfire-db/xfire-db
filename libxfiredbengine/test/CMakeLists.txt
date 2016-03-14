add_executable (xfiredb-unittest
		xfiredb-test.c

		dict/dict-single.c
		dict/dict-concurrent.c
		dict/dict-iterator.c
		dict/dict-database.c

		skiplist/skiplist-single.c
		skiplist/hashmap.c
		skiplist/set.c

		disk/disk-single.c

		core/sleep.c
		core/bitops.c
		core/xfiredb.c
		core/quotearg.c

		bg/bg.c
		bg/bio.c)

target_link_libraries (xfiredb-unittest LINK_PUBLIC xfiredbengine)

