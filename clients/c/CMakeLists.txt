include_directories(${XFIREDB_CLIENT_INCLUDE})

add_library(xfiredb SHARED
	client.c
	result.c
	query.c
	connect.c
	sprintf.c
	split.c
	mem.c)

target_link_libraries(xfiredb LINK_PUBLIC ${OPENSSL_LIB})
add_subdirectory(test)

install (TARGETS xfiredb
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	RUNTIME DESTINATION bin
	)
