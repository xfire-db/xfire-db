include_directories(${XFIRE_SERVER_INCLUDE_DIR})

# standard functions

include (os/os.cmake)
include (storage/storage.cmake)

# libs
add_library (xfireio SHARED
	sprintf.c
	)

add_library (xfire SHARED
	container.c
	bitops.c
	log.c)

target_link_libraries(xfire LINK_PUBLIC xfirestorage xfireos)

install (TARGETS xfire xfireio
	LIBRARY DESTINATION lib
	)

include (tests/bitops/bitops-test.cmake)
include (tests/rbtree/rb-test.cmake)
include (tests/dict/dict-test.cmake)
include (tests/disk/disk.cmake)
include (tests/bg/bg.cmake)

