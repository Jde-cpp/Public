cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( ../../../build/functions.cmake )
project( Jde.Crypto.lib VERSION ${JDE_VERSION} LANGUAGES CXX )
include( ../../../build/dependencies.cmake )

if( WIN32 )
	if( ${OPENSSL_ROOT_DIR} STREQUAL "" )
		message( FATAL_ERROR "Set OPENSSL_ROOT_DIR cmake variable" )
	endif()
	set( OPENSSL_USE_STATIC_LIBS ON )
	find_package( OpenSSL REQUIRED )
endif()

set( targetName Jde.Crypto )
add_library( ${targetName} )
compileOptions( ${targetName} )
file(GLOB sources *.cpp )
file(GLOB headers "**/*.h" $ENV{JDE_DIR}/Public/include/jde/crypto )
target_sources( ${targetName} PRIVATE ${sources} ${headers} )

target_link_libraries( ${targetName} OpenSSL::SSL )

target_precompile_headers(${targetName}
  PUBLIC
  <openssl/engine.h>
  <openssl/hmac.h>
  <openssl/err.h>
  <openssl/evp.h>
  <openssl/rsa.h>
	<openssl/x509v3.h>
  <jde/framework.h>
)