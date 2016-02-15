set(CXX_COMPILE_FLAGS
	"-Wall -std=gnu++98 -pthread -lm")
set(C_COMPILE_FLAGS
	"-Wall -std=gnu89 -pthread -lm")

if(${UNIX})
	set(CXX_COMPILE_FLAGS "${CXX_COMPILE_FLAGS} -ldl")
	set(C_COMPILE_FLAGS "${C_COMPILE_FLAGS} -ldl")
endif(${UNIX})

set(DEBUG_FLAGS "")
if(XFIREDB_DEBUG)
	set(DEBUG_FLAGS "-g2")
else(XFIREDB_DEBUG)
	set(DEBUG_FLAGS "-march=native -O2 -fomit-frame-pointer")
endif(XFIREDB_DEBUG)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_COMPILE_FLAGS} ${DEBUG_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${C_COMPILE_FLAGS} ${DEBUG_FLAGS}")
