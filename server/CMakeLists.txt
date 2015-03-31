include_directories(${XFIRE_SERVER_INCLUDE_DIR})

# standard functions
add_library (xfire SHARED
	bitops.c
	)

include (storage/storage.cmake)

add_executable (xfired
		xfired.cc)
target_link_libraries (xfired LINK_PUBLIC xfire)

install (TARGETS xfired xfire
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	)

include (tests/bitops/bitops-test.cmake)
