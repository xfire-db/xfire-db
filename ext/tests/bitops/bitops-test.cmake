add_executable (bitops-test
		tests/bitops/test.c)

target_link_libraries (bitops-test LINK_PUBLIC xfiredb)
