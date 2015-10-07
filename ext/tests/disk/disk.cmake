add_executable (disk-single
		tests/disk/disk-single.c)

target_link_libraries (disk-single LINK_PUBLIC xfirestorage xfireos)
