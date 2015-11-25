include_directories(${XFIRE_CLIENT_INCLUDE})

add_executable (xfiredb-query
	query.c)

target_link_libraries (xfiredb-query LINK_PUBLIC xfiredb)

