include_directories(${XFIREDB_INCLUDE_DIR})

add_executable (xfiredb-dump
	dump.c)

target_link_libraries (xfiredb-dump LINK_PUBLIC xfiredbengine)
install (TARGETS xfiredb-dump
	RUNTIME DESTINATION bin)

