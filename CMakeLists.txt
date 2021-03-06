cmake_minimum_required (VERSION 2.6)
project (XFireDB)
SET(PROJECT_VERSION 1.1.0)
SET(API_VERSION 1.0)

cmake_policy(SET CMP0007 NEW)
INCLUDE (CheckIncludeFiles)

SET(SQLITE3_LIB xfiredb-sqlite3)
include(scripts/target.cmake)
include(scripts/find_deps.cmake)
include(scripts/directories.cmake)
include(scripts/options.cmake)

# configure file
configure_file(
	"${XFIREDB_INCLUDE_DIR}/config.h.in"
	"${XFIREDB_INCLUDE_DIR}/config.h"
	)

configure_file(
	"${XFIREDB_INCLUDE_DIR}/config.h.in"
	"${XFIREDB_CLIENT_INCLUDE}/config.h"
	)
include(scripts/flags.cmake)
include(scripts/link.cmake)
include(scripts/ruby.cmake)

if(XFIREDB_SERVER)
	add_subdirectory(${XFIREDB_SQLITE3_PATH})
	add_subdirectory(${XFIREDB_ENG_PATH})
	add_subdirectory(${XFIREDB_EXTENSION_PATH})
	add_subdirectory(scripts)
endif(XFIREDB_SERVER)

# install ruby files
INSTALL(FILES lib/xfiredb-serv.rb
	DESTINATION ${RUBY_LIBRARY_INSTALL_DIR})
INSTALL(DIRECTORY lib/xfiredb-serv
	DESTINATION ${RUBY_LIBRARY_INSTALL_DIR})
INSTALL(PROGRAMS bin/xfiredb
	DESTINATION bin)

if(XFIREDB_CLIENTS)
	add_subdirectory(${XFIREDB_CLIENTS_PATH})
endif(XFIREDB_CLIENTS)

