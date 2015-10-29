cmake_minimum_required (VERSION 2.6)
project (XFireDB)
SET(PROJECT_VERSION 1.0)
SET(API_VERSION 1.0)

cmake_policy(SET CMP0007 NEW)
INCLUDE (CheckIncludeFiles)

find_library(SQLITE3_LIB
	NAMES sqlite3
)

CHECK_INCLUDE_FILES(sqlite3.h SQLITE3_HEADER)
IF(NOT SQLITE3_HEADER)
	message( FATAL_ERROR "sqlite3.h is not found" )
ENDIF()

include(scripts/directories.cmake)
include(scripts/options.cmake)

# configure file
configure_file(
	"${XFIRE_INCLUDE_DIR}/config.h.in"
	"${XFIRE_INCLUDE_DIR}/config.h"
	)

include(scripts/flags.cmake)
include(scripts/link.cmake)
include(scripts/ruby.cmake)

include_directories(${XFIRE_INCLUDE_DIR})

if(XFIRE_SERVER)
	add_subdirectory(${XFIRE_ENG_PATH})
	add_subdirectory(${XFIRE_EXTENSION_PATH})
endif(XFIRE_SERVER)
