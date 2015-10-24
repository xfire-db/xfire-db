include_directories(${XFIRE_SERVER_INCLUDE_DIR})

ADD_RUBY_EXTENSION(storage_engine
	storage_engine.c
	se_string.c
	se_list.c
	se_hashmap.c)
target_link_libraries(storage_engine LINK_PUBLIC xfiredb)
install(TARGETS storage_engine
	LIBRARY DESTINATION ${RUBY_EXTENSIONS_INSTALL_DIR})

