cmake_minimum_required (VERSION 2.6)
project (XFire)

include(scripts/options.cmake)
include(scripts/directories.cmake)
include(scripts/flags.cmake)
include(scripts/link.cmake)

include_directories(${XFIRE_INCLUDE_DIR})

if(XFIRE_CLIENT)
	add_subdirectory(${XFIRE_CLIENT_PATH})
endif(XFIRE_CLIENT)

if(XFIRE_SERVER)
	add_subdirectory(${XFIRE_SERVER_PATH})
endif(XFIRE_SERVER)

