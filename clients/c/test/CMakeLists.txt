include_directories(${XFIREDB_CLIENT_INCLUDE})

add_executable (xfiredb-query
	query.c)

target_link_libraries (xfiredb-query LINK_PUBLIC xfiredb)

install (TARGETS xfiredb-query
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	RUNTIME DESTINATION bin
	)
