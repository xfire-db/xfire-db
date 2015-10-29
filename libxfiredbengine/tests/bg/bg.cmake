add_executable (bg
		tests/bg/bg.c)

add_executable (bio
		tests/bg/bio.c)

target_link_libraries (bg LINK_PUBLIC xfiredbengine)
target_link_libraries (bio LINK_PUBLIC xfiredbengine)

