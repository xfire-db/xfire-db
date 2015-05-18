#
# Build the engine library
#

add_library(xfireengine SHARED
	engine/core.c
	engine/hash.c
	engine/request.c
	engine/container.c
	engine/sched.c

	include/xfire/request.h
	include/xfire/engine.h
	include/xfire/hash.h
	include/xfire/container.h
	)

target_link_libraries(xfireengine LINK_PUBLIC xfire xfireos xfirestorage)

install (TARGETS xfireengine
	LIBRARY DESTINATION lib
	)

include (engine/rbdb/rbdb.cmake)
