add_executable (rb-test
		tests/rbtree/rb-test.c)

add_executable (concurrent-rb-test
		tests/rbtree/rb-concurrent.c)

add_executable (rb-remove
		tests/rbtree/rb-remove.c)

target_link_libraries (rb-test LINK_PUBLIC xfire-storage)
target_link_libraries (concurrent-rb-test LINK_PUBLIC xfire-storage xfire-os)
target_link_libraries (rb-remove LINK_PUBLIC xfire-storage xfire-os)
