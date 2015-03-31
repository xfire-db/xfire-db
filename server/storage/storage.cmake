#
# Build the storage library
#

add_library(bststorage SHARED
	storage/binarytreekey.cc
	storage/binarytreevalue.cc
	storage/binarytreenode.cc
	storage/binarytree.cc
	)
