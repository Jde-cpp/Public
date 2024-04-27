cmake_minimum_required(VERSION 3.16)
project(Jde.Crypto)
set( CMAKE_MODULE_PATH $ENV{JDE_DIR}/Public/build )
include(common)
link_directories( ${fmt_DIR}/../.. )
link_directories( ${Protobuf_DIR}/../.. )
#######################################################
set(CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS} "-Wl,-rpath=$ORIGIN")
add_definitions( -DJde_CRYPTO_EXPORTS )

find_package(OpenSSL REQUIRED)

add_library( ${CMAKE_PROJECT_NAME} SHARED OpenSsl.cpp OpenSslInternal.cpp )
#######################################################
target_link_libraries( ${CMAKE_PROJECT_NAME} Threads::Threads )
target_link_libraries( ${CMAKE_PROJECT_NAME} debug libfmtd.a optimized libfmt.a )
#target_link_libraries( ${CMAKE_PROJECT_NAME} libcrypto.a )
target_link_libraries(${CMAKE_PROJECT_NAME} OpenSSL::SSL)
#######################################################
target_precompile_headers(${CMAKE_PROJECT_NAME}
  PRIVATE
  <openssl/engine.h>
  <openssl/hmac.h>
  <openssl/err.h>
  <openssl/evp.h>
  <openssl/rsa.h>

  <jde/TypeDefs.h>
	<jde/Exception.h>
  <jde/crypto/exports.h>
)
