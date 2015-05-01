add_library (xfire SHARED
	lib/bitops-atomic.c
	lib/bitops.c
	lib/mem.c
	)

install (TARGETS xfire
	LIBRARY DESTINATION lib
	)
