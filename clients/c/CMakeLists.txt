include_directories(${XFIREDB_CLIENT_INCLUDE})

add_library(xfiredb SHARED
	client.c
	result.c
	query.c
	connect.c
	sprintf.c
	split.c
	mem.c)

if(${UNIX})
	target_link_libraries(xfiredb LINK_PUBLIC ${OPENSSL_LIB})
else(${UNIX})
	target_link_libraries(xfiredb LINK_PUBLIC ${OPENSSL_LIB} ${CRYPTO_LIB} ${WS32_LIB})
endif(${UNIX})

add_subdirectory(test)

install (TARGETS xfiredb
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	RUNTIME DESTINATION bin
	)
