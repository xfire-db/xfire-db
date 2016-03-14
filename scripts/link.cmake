if(${UNIX})
	SET (CMAKE_SKIP_BUILD_RPATH FALSE)

	SET (CMAKE_INSTALL_PREFIX $ENV{CMAKE_INSTALL_PREFIX})

# Don't use the RPATH while building
	SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
	SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

# add automatically determined parts of the RPATH
	set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# the RPATH to be used when installing, but only
# if it's not a system dir
	set(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib"
		isSystemDir)

	if("${isSystemDir}" STREQUAL "-1")
		set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
	endif("${isSystemDir}" STREQUAL "-1")
endif(${UNIX})

if("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
	set(XFIREDB_SQLITE_LIB sqlite3)
endif("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")

