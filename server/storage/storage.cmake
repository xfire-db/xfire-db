#
# Build the storage library
#

add_library(xfire-storage SHARED
	storage/rbtree.c
	storage/database.c
	storage/hashmap.c

	include/xfire/rbtree.h
	include/xfire/database.h
	include/xfire/hashmap.h
	)

target_link_libraries(xfire-storage LINK_PUBLIC xfire xfire-os)

install (TARGETS xfire-storage
	LIBRARY DESTINATION lib
	)
