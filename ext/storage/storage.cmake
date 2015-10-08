#
# Build the storage library
#

add_library(xfirestorage SHARED
	storage/dict.c
	storage/disk.c
	storage/rbtree.c
	storage/list.c
	storage/database.c
	storage/string.c

	include/xfire/dict.h
	include/xfire/disk.h
	include/xfire/rbtree.h
	include/xfire/list.h
	include/xfire/database.h
	include/xfire/string.h
	)

target_link_libraries(xfirestorage LINK_PUBLIC xfire xfireos sqlite3)

install (TARGETS xfirestorage
	LIBRARY DESTINATION lib
	)
