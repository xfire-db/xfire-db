include_directories(${XFIRE_SERVER_INCLUDE_DIR})

# standard functions

if("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
	set(XFIRE_OS_FILES
		os/bg.c
		os/linux.c
		os/mem.c
	)
endif("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")

# libxfiredb
add_library(xfiredbengine SHARED
	# storage files	
	storage/dict.c
	storage/database.c
	storage/disk.c
	storage/hashmap.c
	storage/bio.c
	storage/rbtree.c
	storage/list.c
	storage/string.c
	storage/container.c

	# os files
	${XFIRE_OS_FILES}

	# core files
	bitops-atomic.c
	bitops.c
	xfiredb.c
	log.c
	sprintf.c)


target_link_libraries(xfiredbengine LINK_PUBLIC ${SQLITE3_LIB})

install (TARGETS xfiredbengine
	LIBRARY DESTINATION lib
	)

if(XFIRE_DEBUG)
	include (tests/rbtree/rb-test.cmake)
	include (tests/dict/dict-test.cmake)
	include (tests/disk/disk.cmake)
	include (tests/bg/bg.cmake)
	include (tests/core/core.cmake)
endif(XFIRE_DEBUG)
