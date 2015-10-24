add_executable (xfiredb-test
		tests/core/xfiredb.c)
add_executable (db-fill
		tests/core/fill-db.c)
add_executable (db-load
		tests/core/db-load.c)

add_executable (bitops-single
		tests/core/bitops.c)

target_link_libraries (db-fill LINK_PUBLIC xfiredb)
target_link_libraries (db-load LINK_PUBLIC xfiredb)
target_link_libraries (xfiredb-test LINK_PUBLIC xfiredb)
target_link_libraries (bitops-single LINK_PUBLIC xfiredb)

