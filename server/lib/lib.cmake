add_library (xfire SHARED
	lib/bitops-atomic.c
	lib/bitops.c
	lib/log.c
	lib/mem.c

	include/xfire/log.h
	)

install (TARGETS xfire
	LIBRARY DESTINATION lib
	)