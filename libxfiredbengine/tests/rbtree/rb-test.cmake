add_executable (rb-concurrent
		tests/unit_test.c
		tests/rbtree/rb-concurrent.c)

add_executable (rb-single
		tests/unit_test.c
		tests/rbtree/rb-single.c)

add_executable (rb-hashmap
		tests/unit_test.c
		tests/rbtree/hashmap.c)

target_link_libraries (rb-single LINK_PUBLIC xfiredbengine)
target_link_libraries (rb-concurrent LINK_PUBLIC xfiredbengine)
target_link_libraries (rb-hashmap LINK_PUBLIC xfiredbengine)
