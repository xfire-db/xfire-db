MESSAGE( STATUS "looking for libssl")
find_library(OPENSSL_LIB
	NAMES ssl
)
MESSAGE( STATUS "Found openSSL: " ${OPENSSL_LIB})

CHECK_INCLUDE_FILES(openssl/ssl.h SSL_HEADER)
CHECK_INCLUDE_FILES(stdlib.h STDLIB_HEADER)
CHECK_INCLUDE_FILES(stdint.h STDINT_HEADER)
CHECK_INCLUDE_FILES(stdio.h STDIO_HEADER)
CHECK_INCLUDE_FILES(stdarg.h STDARG_HEADER)
CHECK_INCLUDE_FILES(string.h STRING_HEADER)
CHECK_INCLUDE_FILES(assert.h ASSERT_HEADER)
CHECK_INCLUDE_FILES(unistd.h UNISTD_HEADER)

IF(NOT SSL_HEADER)
	message( FATAL_ERROR "openssl/ssl.h is not found" )
ENDIF()

IF(NOT UNISTD_HEADER)
	message( FATAL_ERROR "unistd.h is not found" )
endif()

IF(NOT STRING_HEADER)
	message( FATAL_ERROR "string.h is not found" )
ENDIF()

IF(NOT ASSERT_HEADER)
	message( FATAL_ERROR "assert.h is not found" )
ENDIF()

IF(NOT STDINT_HEADER)
	message( FATAL_ERROR "stdint.h is not found" )
ENDIF()

IF(NOT STDLIB_HEADER)
	message( FATAL_ERROR "stdlib.h is not found" )
ENDIF()

IF(NOT STDIO_HEADER)
	message( FATAL_ERROR "stdio.h is not found" )
ENDIF()

