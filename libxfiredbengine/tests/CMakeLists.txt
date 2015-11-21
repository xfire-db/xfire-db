add_executable (xfiredb-unittest
		xfiredb-test.c

		dict/dict-single.c
		dict/dict-concurrent.c
		dict/dict-iterator.c
		dict/dict-database.c)

target_link_libraries (xfiredb-unittest LINK_PUBLIC xfiredbengine)

