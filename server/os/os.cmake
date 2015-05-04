#
# OS specific library
#

if("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
	set(XFIRE_OS_FILES
		os/linux.c
	)
endif("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")

add_library(xfireos SHARED
	    ${XFIRE_OS_FILES}
	    )


install (TARGETS xfireos
	LIBRARY DESTINATION lib
	)
