#
# Build the storage library
#

add_library(bststorage SHARED
	storage/rbtree.c
	storage/database.c
	storage/hashmap.c

	include/xfire/rbtree.h
	include/xfire/database.h
	include/xfire/hashmap.h
	)

target_link_libraries(bststorage LINK_PUBLIC xfire)

install (TARGETS bststorage
	LIBRARY DESTINATION lib
	)
