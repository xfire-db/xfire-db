add_executable (dict-single
		tests/dict/dict-single.c)

target_link_libraries (dict-single LINK_PUBLIC xfirestorage)

