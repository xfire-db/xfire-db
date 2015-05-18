#
# Build the engine library
#

add_library(xfirerbdb SHARED
	engine/rbdb/rb_db.c

	include/xfire/rb_db.h
	)

target_link_libraries(xfirerbdb LINK_PUBLIC xfire xfireos xfirestorage)

install (TARGETS xfirerbdb
	LIBRARY DESTINATION lib
	)
