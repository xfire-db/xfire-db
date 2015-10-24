add_executable (rb-insert
		tests/rbtree/rb-insert.c)

add_executable (rb-concurrent
		tests/rbtree/rb-concurrent.c)

add_executable (rb-remove
		tests/unit_test.c
		tests/rbtree/rb-remove.c)

add_executable (hashmap
		tests/rbtree/hashmap.c)

target_link_libraries (rb-remove LINK_PUBLIC xfiredb)
target_link_libraries (rb-concurrent LINK_PUBLIC xfiredb)
target_link_libraries (rb-insert LINK_PUBLIC xfiredb)
target_link_libraries (hashmap LINK_PUBLIC xfiredb)
