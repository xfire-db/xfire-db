add_executable (dict-single
		tests/dict/dict-single.c)

target_link_libraries (rb-insert LINK_PUBLIC xfirestorage)
