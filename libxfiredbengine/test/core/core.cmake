add_executable (xfiredb-test
		unit_test.c
		core/xfiredb.c)
add_executable (db-load
		core/db-load.c)

add_executable (test-quotearg
		unit_test.c
		core/quotearg.c)

add_executable (bitops-single
		core/bitops.c)

target_link_libraries (test-quotearg LINK_PUBLIC xfiredbengine)
target_link_libraries (db-load LINK_PUBLIC xfiredbengine)
target_link_libraries (xfiredb-test LINK_PUBLIC xfiredbengine)
target_link_libraries (bitops-single LINK_PUBLIC xfiredbengine)

