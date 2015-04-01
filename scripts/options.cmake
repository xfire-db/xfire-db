option (XFIRE_SERVER
	"Set to to true if the XFire server should be build"
	[false])
option (XFIRE_CLIENT
	"Set to true if the XFire client should be build"
	[false])

option (XFIRE_DEBUG
	"Set to true if debugging options should be enabled"
	[false])

set(HAVE_DBG "")
if(XFIRE_DEBUG)
	set(HAVE_DBG "#define HAVE_DBG")
endif(XFIRE_DEBUG)
