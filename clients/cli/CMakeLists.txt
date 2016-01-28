include_directories(${XFIREDB_CLIENT_INCLUDE})

add_executable(xfiredb-cli
		cli.c
		cli.h
	      )
install(TARGETS xfiredb-cli
	RUNTIME DESTINATION bin
       )
