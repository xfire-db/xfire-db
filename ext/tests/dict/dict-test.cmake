add_executable (dict-single
		tests/dict/dict-single.c)

add_executable (dict-iterator
		tests/dict/dict-iterator.c)

add_executable (dict-concurrent
		tests/dict/dict-concurrent.c)

add_executable (dict-database
		tests/dict/dict-database.c)

target_link_libraries (dict-single LINK_PUBLIC xfirestorage)
target_link_libraries (dict-iterator LINK_PUBLIC xfirestorage)
target_link_libraries (dict-concurrent LINK_PUBLIC xfirestorage xfireos)
target_link_libraries (dict-database LINK_PUBLIC xfirestorage xfireos)

