add_executable (rb-test
		tests/rbtree/rb-test.c)

target_link_libraries (rb-test LINK_PUBLIC bststorage)
