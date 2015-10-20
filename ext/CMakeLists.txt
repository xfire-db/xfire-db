include_directories(${XFIRE_SERVER_INCLUDE_DIR})

# standard functions

if("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
	set(XFIRE_OS_FILES
		os/bg.c
		os/linux.c
		os/mem.c
		os/bitops-atomic.c
	)
endif("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")

# libxfiredb
add_library(xfiredb SHARED
	# storage files	
	storage/dict.c
	storage/database.c
	storage/disk.c
	storage/hashmap.c
	storage/bio.c
	storage/rbtree.c
	storage/list.c
	storage/string.c

	# os files
	${XFIRE_OS_FILES}

	# core files
	container.c
	bitops.c
	xfiredb.c
	log.c
	sprintf.c)


target_link_libraries(xfiredb LINK_PUBLIC ${XFIRE_SQLITE_LIB})

install (TARGETS xfiredb
	LIBRARY DESTINATION lib
	)

include (tests/bitops/bitops-test.cmake)
include (tests/rbtree/rb-test.cmake)
include (tests/dict/dict-test.cmake)
include (tests/disk/disk.cmake)
include (tests/bg/bg.cmake)
include (tests/core/core.cmake)

