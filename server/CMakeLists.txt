include_directories(${XFIRE_SERVER_INCLUDE_DIR})

# standard functions
add_library (xfire SHARED
	bitops.c
	)

# bst
include (storage/storage.cmake)
include (os/os.cmake)

add_executable (xfired
		xfired.cc)

install (TARGETS xfired xfire
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	)

include (tests/bitops/bitops-test.cmake)
include (tests/rbtree/rb-test.cmake)
