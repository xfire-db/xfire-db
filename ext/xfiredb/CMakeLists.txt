include_directories(${XFIRE_SERVER_INCLUDE_DIR})

ADD_RUBY_EXTENSION(storage_engine
	database.c
	list.c
	hashmap.c
	storage_engine.c
	)

target_link_libraries(storage_engine LINK_PUBLIC xfiredb)
install(TARGETS storage_engine
	LIBRARY DESTINATION ${RUBY_EXTENSIONS_INSTALL_DIR})

