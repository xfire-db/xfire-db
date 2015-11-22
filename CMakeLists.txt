cmake_minimum_required (VERSION 2.6)
project (XFireDB)
SET(PROJECT_VERSION 1.0)
SET(API_VERSION 1.0)

cmake_policy(SET CMP0007 NEW)
INCLUDE (CheckIncludeFiles)

include(scripts/find_deps.cmake)
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

if(XFIRE_SERVER)
	add_subdirectory(${XFIRE_ENG_PATH})
	add_subdirectory(${XFIRE_EXTENSION_PATH})
	add_subdirectory(scripts)
endif(XFIRE_SERVER)

