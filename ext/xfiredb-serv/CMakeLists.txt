include_directories(${XFIREDB_INCLUDE_DIR} ${XFIREDB_SQLITE_INCLUDE_DIR})

ADD_RUBY_EXTENSION(storage_engine
	database.c
	string.c
	set.c
	list.c
	hashmap.c
	storage_engine.c
	log.c
	digest.c
	)

target_link_libraries(storage_engine LINK_PUBLIC xfiredbengine)

install(TARGETS storage_engine
	LIBRARY DESTINATION ${RUBY_EXTENSIONS_INSTALL_DIR}/xfiredb-serv)

