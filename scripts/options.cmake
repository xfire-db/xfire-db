option (X64
	"Set to true if compiling for a 64-bit machine"
	true)

option (XFIREDB_SERVER
	"Set to to true if the XFire server should be build and installed"
	true)

option (XFIREDB_CLIENT
	"Set to true if the XFire client library's should be built and installed"
	true)

option (XFIREDB_DEBUG
	"Set to true if debugging options should be enabled"
	[false])


set (RUBY_RBCONF "" CACHE STRING
	"Ruby config file.")

set (STACK_SIZE "2097152" CACHE STRING
	"Default stack size.")	
set (DATA_PATH "$ENV{HOME}/.xfire" CACHE STRING
	"Directory to store the debugging disk.")

set(HAVE_DBG "")
if(XFIREDB_DEBUG)
	set(HAVE_DBG "#define HAVE_DBG")
endif(XFIREDB_DEBUG)

set(HAVE_X64 "")
if(X64)
	set(HAVE_X64 "#define HAVE_X64")
endif(X64)

set(HAVE_UNIX "")
set(HAVE_WINDOWS "")
if("${CMAKE_SYSTEM_NAME}" MATCHES "Windows")
	set(HAVE_WINDOWS "#define HAVE_WINDOWS")
elseif(${UNIX})
	set(HAVE_UNIX "#define HAVE_UNIX")
endif()

