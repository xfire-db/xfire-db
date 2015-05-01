include_directories(${XFIRE_SERVER_INCLUDE_DIR})

# standard functions

include (os/os.cmake)
include (storage/storage.cmake)
include (engine/engine.cmake)

# libs
include (lib/lib.cmake)

add_executable (xfired
		xfired.cc)

install (TARGETS xfired
	RUNTIME DESTINATION bin
	)

include (tests/bitops/bitops-test.cmake)
include (tests/rbtree/rb-test.cmake)
