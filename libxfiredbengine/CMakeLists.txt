if(XFIREDB_PTHREAD)
	set(XFIREDB_OS_FILES
		os/pthread.c
	)
endif(XFIREDB_PTHREAD)

include_directories(${XFIREDB_INCLUDE_DIR})

set(XFIREDB_OS_FILES
	${XFIREDB_OS_FILES}
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
	${XFIREDB_OS_FILES}
	os/time.c

	# core files
	crc16.c
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

if(XFIREDB_DEBUG)
	add_subdirectory(test)
endif(XFIREDB_DEBUG)

