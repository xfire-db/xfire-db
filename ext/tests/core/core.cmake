add_executable (xfiredb-test
		tests/core/xfiredb.c)

target_link_libraries (xfiredb-test LINK_PUBLIC xfiredb)

