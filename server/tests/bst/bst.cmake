add_executable (bst-test
		tests/bst/bst.cc)

target_link_libraries (bst-test LINK_PUBLIC bststorage)

install (TARGETS bst-test
	RUNTIME DESTINATION bin
	)
