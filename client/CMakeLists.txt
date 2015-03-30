add_executable (xfire-cli
		xfire.cc
		xfire-cli.cc)

install (TARGETS xfire-cli
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	)
