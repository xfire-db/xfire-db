add_executable (bg
		tests/bg/bg.c)
target_link_libraries (bg LINK_PUBLIC xfire xfirestorage xfireos)

