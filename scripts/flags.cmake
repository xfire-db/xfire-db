set(CXX_COMPILE_FLAGS
	"-Wall -std=gnu++98 -lm -ldl")
set(C_COMPILE_FLAGS
	"-Wall -std=gnu89 -lm -ldl")

if("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
	set(C_COMPILE_FLAGS "${C_COMPILE_FLAGS} -pthread")
	set(CXX_COMPILE_FLAGS "${CXX_COMPILE_FLAGS} -pthread")
endif("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")

set(DEBUG_FLAGS "")
if(XFIREDB_DEBUG)
	set(DEBUG_FLAGS "-g2")
else(XFIREDB_DEBUG)
	set(DEBUG_FLAGS "-march=native -O2 -fomit-frame-pointer")
endif(XFIREDB_DEBUG)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_COMPILE_FLAGS} ${DEBUG_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${C_COMPILE_FLAGS} ${DEBUG_FLAGS}")
