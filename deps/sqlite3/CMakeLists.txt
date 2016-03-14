include_directories(${XFIREDB_INCLUDE_DIR})
add_library(xfiredb-sqlite3 SHARED
	sqlite3.c)

INSTALL(TARGETS xfiredb-sqlite3
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	RUNTIME DESTINATION bin
	)
