option (XFIRE_SERVER
	"Set to to true if the XFire server should be build"
	[false])
option (XFIRE_CLIENT
	"Set to true if the XFire client should be build"
	[false])

option (XFIRE_DEBUG
	"Set to true if debugging options should be enabled"
	[false])

option (RECURSION
	"Set to true if search functions should use recursion."
	[true])

set(HAVE_DBG "")
if(XFIRE_DEBUG)
	set(HAVE_DBG "#define HAVE_DBG")
endif(XFIRE_DEBUG)


if("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
	set(HAVE_LINUX "#define HAVE_LINUX")
endif("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")

set(HAVE_RECURSION "")
if(RECURSION)
	set(HAVE_RECURSION "#define HAVE_RECURSION")
endif(RECURSION)
