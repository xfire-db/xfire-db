MESSAGE( STATUS "looking for libssl")
find_library(OPENSSL_LIB
	NAMES libssl.so libssl.a ssl
	HINTS ${CMAKE_FIND_ROOT_PATH}/lib)

MESSAGE( STATUS "Found openSSL: " ${OPENSSL_LIB})

CHECK_INCLUDE_FILES(openssl/ssl.h SSL_HEADER)

IF(NOT SSL_HEADER)
	message( FATAL_ERROR "openssl/ssl.h is not found" )
ENDIF()
add_subdirectory(c)
