add_executable (hash-test
		tests/engine/hash.c)

target_link_libraries (hash-test LINK_PUBLIC xfireengine)
