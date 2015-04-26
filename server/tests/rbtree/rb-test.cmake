add_executable (rb-insert
		tests/rbtree/rb-insert.c)

add_executable (rb-concurrent
		tests/rbtree/rb-concurrent.c)

add_executable (rb-remove
		tests/rbtree/rb-remove.c)

target_link_libraries (rb-remove LINK_PUBLIC xfirestorage)
target_link_libraries (rb-concurrent LINK_PUBLIC xfirestorage xfireos)
target_link_libraries (rb-insert LINK_PUBLIC xfirestorage)
