add_executable (db-load
		core/db-load.c)

add_executable (test-quotearg
		unit_test.c
		core/quotearg.c)

target_link_libraries (test-quotearg LINK_PUBLIC xfiredbengine)
target_link_libraries (db-load LINK_PUBLIC xfiredbengine)

