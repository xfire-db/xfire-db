add_executable (hash-test
		tests/engine/hash.c)

add_executable (engine-test
		tests/engine/core.c)
target_link_libraries (hash-test LINK_PUBLIC xfireengine)
target_link_libraries (engine-test LINK_PUBLIC xfireengine xfirestorage xfirerbdb)
