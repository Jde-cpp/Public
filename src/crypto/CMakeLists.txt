cmake_minimum_required(VERSION 3.20.0)
set( CMAKE_MODULE_PATH $ENV{JDE_DIR}/Public/build )
include(common)
findBoost()

if( ${VCPKG} )
	SET(CMAKE_TOOLCHAIN_FILE "$ENV{REPO_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"  CACHE STRING "Vcpkg toolchain file")
endif()
project(Jde.Crypto)
include(compiler)
#######################################################
find_package( OpenSSL REQUIRED )
if( MSVC )
	include_directories( ${OPENSSL_INCLUDE_DIR} )
	link_directories( $ENV{JDE_DIR}/Framework/source/.build/.bin/${outDir} )
else()
	link_directories( ${fmt_DIR}/../.. )
	link_directories( ${Protobuf_DIR}/../.. )
	add_definitions( -DJde_Web_Client_EXPORTS )
endif()
#######################################################
add_library( ${CMAKE_PROJECT_NAME} SHARED OpenSsl.cpp OpenSslInternal.cpp CryptoSettings.cpp )
#######################################################
if(MSVC)
	find_package(fmt)
	target_link_libraries( ${CMAKE_PROJECT_NAME} fmt::fmt )
else()
	add_definitions( -DJde_Crypto_EXPORTS )
	target_link_libraries( ${CMAKE_PROJECT_NAME} Threads::Threads )
	target_link_libraries( ${CMAKE_PROJECT_NAME} debug libfmtd.a optimized libfmt.a )
	#target_link_libraries( ${CMAKE_PROJECT_NAME} libcrypto.a )
endif()
target_link_libraries( ${CMAKE_PROJECT_NAME} OpenSSL::SSL )

#######################################################

target_precompile_headers(${CMAKE_PROJECT_NAME}
  PRIVATE
  <openssl/engine.h>
  <openssl/hmac.h>
  <openssl/err.h>
  <openssl/evp.h>
  <openssl/rsa.h>
	<openssl/x509v3.h>

  <jde/TypeDefs.h>
	<jde/log/Log.h>
	<jde/Exception.h>
  <jde/crypto/exports.h>
)
