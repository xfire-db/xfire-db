include_directories(${XFIRE_SERVER_INCLUDE_DIR})

# standard functions

include (os/os.cmake)
include (storage/storage.cmake)

# libs

add_library (xfire SHARED
	bitops-atomic.c
	bitops.c
	log.c)

install (TARGETS xfire
	LIBRARY DESTINATION lib
	)

include (tests/bitops/bitops-test.cmake)
include (tests/rbtree/rb-test.cmake)
include (tests/dict/dict-test.cmake)
include (tests/disk/disk.cmake)
