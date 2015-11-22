if("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
	set(XFIRE_OS_FILES
		os/linux.c
	)
endif("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")

include_directories(${XFIRE_SERV_INCLUDE_DIR})

set(XFIRE_OS_FILES
	${XFIRE_OS_FILES}
	os/atomic.c
	os/mem.c
	os/bg.c)

# libxfiredb
add_library(xfiredbengine SHARED
	# storage files	
	storage/dict.c
	storage/database.c
	storage/disk.c
	storage/set.c
	storage/hashmap.c
	storage/bio.c
	storage/rbtree.c
	storage/list.c
	storage/string.c
	storage/container.c

	# os files
	${XFIRE_OS_FILES}
	os/sleep.c

	# core files
	bitops-atomic.c
	bitops.c
	xfiredb.c
	log.c
	sprintf.c
	quotearg.c)


target_link_libraries(xfiredbengine LINK_PUBLIC ${SQLITE3_LIB})

install (TARGETS xfiredbengine
	LIBRARY DESTINATION lib
	)

if(XFIRE_DEBUG)
	add_subdirectory(test)
endif(XFIRE_DEBUG)

