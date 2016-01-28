include_directories(${XFIREDB_CLIENT_INCLUDE})

add_executable(xfiredb-cli
		cli.c
		cli.h
	      )

target_link_libraries(xfiredb-cli LINK_PUBLIC xfiredb ${OPENSSL_LIB})
install(TARGETS xfiredb-cli
	RUNTIME DESTINATION bin
       )
