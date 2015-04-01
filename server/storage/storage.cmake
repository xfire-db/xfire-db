#
# Build the storage library
#

add_library(bststorage SHARED
	storage/binarytreenode.cc
	storage/binarytree.cc
	storage/rbtreenode.cc
	storage/rbtree.cc
	storage/testdatabase.cc

	"${XFIRE_SERVER_INCLUDE_DIR}/xfire/binarytree.h"
	"${XFIRE_SERVER_INCLUDE_DIR}/xfire/binarytreenode.h"
	"${XFIRE_SERVER_INCLUDE_DIR}/xfire/rbtreenode.h"
	"${XFIRE_SERVER_INCLUDE_DIR}/xfire/rbtree.h"
	"${XFIRE_SERVER_INCLUDE_DIR}/xfire/database.h"
	"${XFIRE_SERVER_INCLUDE_DIR}/xfire/testdatabase.h"
	)

target_link_libraries(bststorage LINK_PUBLIC xfire)

install (TARGETS bststorage
	LIBRARY DESTINATION lib
	)
