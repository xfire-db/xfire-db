add_executable (dict-single
		tests/unit_test.c
		tests/dict/dict-single.c)

add_executable (dict-iterator
		tests/unit_test.c
		tests/dict/dict-iterator.c)

add_executable (dict-concurrent
		tests/unit_test.c
		tests/dict/dict-concurrent.c)

add_executable (dict-database
		tests/unit_test.c
		tests/dict/dict-database.c)

target_link_libraries (dict-single LINK_PUBLIC xfiredb)
target_link_libraries (dict-iterator LINK_PUBLIC xfiredb)
target_link_libraries (dict-concurrent LINK_PUBLIC xfiredb)
target_link_libraries (dict-database LINK_PUBLIC xfiredb)

