cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( ../../../build/functions.cmake )
project( Jde.Crypto.lib VERSION ${JDE_VERSION} LANGUAGES CXX )
include( ../../../build/dependencies.cmake )

set( targetName Jde.Crypto )
add_library( ${targetName} )
compileOptions( ${targetName} )
file(GLOB sources *.cpp )
file(GLOB headers "**/*.h" $ENV{JDE_DIR}/Public/include/jde/crypto )
target_sources( ${targetName} PRIVATE ${sources} ${headers} )

if( WIN32 )
	target_link_libraries( ${targetName} libcrypto libssl )
else()
	target_link_libraries( ${targetName} OpenSSL::SSL )
endif()

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