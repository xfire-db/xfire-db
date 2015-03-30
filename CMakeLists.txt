cmake_minimum_required (VERSION 2.6)
project (XFire)

option (XFIRE_SERVER
	"Set to to true if the XFire server should be build"
	[false])
option (XFIRE_CLIENT
	"Set to true if the XFire client should be build"
	[false])

set(XFIRE_INCLUDE_DIR include)
set(XFIRE_CLIENT_PATH client)
set(XFIRE_SERVER_PATH server)

include_directories(${XFIRE_INCLUDE_DIR})

if(XFIRE_CLIENT)
	add_subdirectory(${XFIRE_CLIENT_PATH})
endif(XFIRE_CLIENT)

if(XFIRE_SERVER)
	add_subdirectory(${XFIRE_SERVER_PATH})
endif(XFIRE_SERVER)

