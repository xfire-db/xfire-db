add_executable (disk-single
	tests/disk/disk-single.c)

add_executable (disk-dump
	tests/disk/disk-dump.c)

target_link_libraries (disk-single LINK_PUBLIC xfiredbengine)
target_link_libraries (disk-dump LINK_PUBLIC xfiredbengine)

