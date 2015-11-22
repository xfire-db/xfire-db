include_directories(${XFIRE_CLIENT_INCLUDE})

add_library(xfiredb SHARED
	client.c
	result.c
	query.c
	connect.c)

install (TARGETS xfiredb
	LIBRARY DESTINATION lib
	)
