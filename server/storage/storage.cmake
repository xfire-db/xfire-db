#
# Build the storage library
#

add_library(xfirestorage SHARED
	storage/dict.c
	storage/rbtree.c
	storage/list.c
	storage/database.c
	storage/hashmap.c
	storage/string.c

	include/xfire/dict.h
	include/xfire/rbtree.h
	include/xfire/list.h
	include/xfire/database.h
	include/xfire/hashmap.h
	include/xfire/string.h
	include/xfire/rb_db.h
	)

target_link_libraries(xfirestorage LINK_PUBLIC xfire xfireos)

install (TARGETS xfirestorage
	LIBRARY DESTINATION lib
	)
