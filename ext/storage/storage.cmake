#
# Build the storage library
#

add_library(xfirestorage SHARED
	storage/dict.c
	storage/database.c
	storage/disk.c
	storage/hashmap.c
	storage/bio.c
	storage/rbtree.c
	storage/list.c
	storage/string.c

	include/xfire/dict.h
	include/xfire/disk.h
	include/xfire/rbtree.h
	include/xfire/list.h
	include/xfire/database.h
	include/xfire/string.h
	)

target_link_libraries(xfirestorage LINK_PUBLIC xfireos xfireio sqlite3)

install (TARGETS xfirestorage
	LIBRARY DESTINATION lib
	)
