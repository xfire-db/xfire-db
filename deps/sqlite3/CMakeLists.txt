include_directories(${XFIREDB_INCLUDE_DIR})
add_library(xfiredb-sqlite3 SHARED
	sqlite3.c)
