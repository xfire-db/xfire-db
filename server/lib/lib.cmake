add_library (xfire SHARED
	lib/bitops.c
	)

install (TARGETS xfire
	LIBRARY DESTINATION lib
	)
