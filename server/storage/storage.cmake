#
# Build the storage library
#

add_library(xfirestorage SHARED
	storage/rbtree.c
	storage/list.c
	storage/database.c
	storage/hashmap.c

	include/xfire/rbtree.h
	include/xfire/list.h
	include/xfire/database.h
	include/xfire/hashmap.h
	)

target_link_libraries(xfirestorage LINK_PUBLIC xfire xfireos)

install (TARGETS xfirestorage
	LIBRARY DESTINATION lib
	)
