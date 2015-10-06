add_executable (dict-single
		tests/dict/dict-single.c)

add_executable (dict-iterator
		tests/dict/dict-iterator.c)

add_executable (dict-concurrent
		tests/dict/dict-concurrent.c)

target_link_libraries (dict-single LINK_PUBLIC xfirestorage)
target_link_libraries (dict-iterator LINK_PUBLIC xfirestorage)
target_link_libraries (dict-concurrent LINK_PUBLIC xfirestorage xfireos)

