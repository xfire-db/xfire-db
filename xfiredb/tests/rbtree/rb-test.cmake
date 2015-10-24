add_executable (rb-concurrent
		tests/rbtree/rb-concurrent.c)

add_executable (rb-single
		tests/unit_test.c
		tests/rbtree/rb-single.c)

add_executable (rb-hashmap
		tests/rbtree/hashmap.c)

target_link_libraries (rb-single LINK_PUBLIC xfiredb)
target_link_libraries (rb-concurrent LINK_PUBLIC xfiredb)
target_link_libraries (rb-hashmap LINK_PUBLIC xfiredb)
