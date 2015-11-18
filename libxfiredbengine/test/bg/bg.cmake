add_executable (bg
		bg/bg.c)

add_executable (bio
		bg/bio.c)

target_link_libraries (bg LINK_PUBLIC xfiredbengine)
target_link_libraries (bio LINK_PUBLIC xfiredbengine)

