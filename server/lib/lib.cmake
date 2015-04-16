add_library (xfire SHARED
	lib/bitops-atomic.c
	lib/bitops.c
	)

install (TARGETS xfire
	LIBRARY DESTINATION lib
	)
