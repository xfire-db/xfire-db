add_executable (dict-single
		unit_test.c
		dict/dict-single.c)

add_executable (dict-iterator
		unit_test.c
		dict/dict-iterator.c)

add_executable (dict-concurrent
		unit_test.c
		dict/dict-concurrent.c)

add_executable (dict-database
		unit_test.c
		dict/dict-database.c)

target_link_libraries (dict-single LINK_PUBLIC xfiredbengine)
target_link_libraries (dict-iterator LINK_PUBLIC xfiredbengine)
target_link_libraries (dict-concurrent LINK_PUBLIC xfiredbengine)
target_link_libraries (dict-database LINK_PUBLIC xfiredbengine)

