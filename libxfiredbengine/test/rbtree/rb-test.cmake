add_executable (rb-concurrent
		unit_test.c
		rbtree/rb-concurrent.c)

add_executable (rb-single
		unit_test.c
		rbtree/rb-single.c)

add_executable (rb-hashmap
		unit_test.c
		rbtree/hashmap.c)

add_executable (rb-set
		unit_test.c
		rbtree/set.c)

target_link_libraries (rb-set LINK_PUBLIC xfiredbengine)
target_link_libraries (rb-single LINK_PUBLIC xfiredbengine)
target_link_libraries (rb-concurrent LINK_PUBLIC xfiredbengine)
target_link_libraries (rb-hashmap LINK_PUBLIC xfiredbengine)
