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
add_library( ${CMAKE_PROJECT_NAME} )
file(GLOB sources *.cpp )
file(GLOB headers "**/*.h" $ENV{JDE_DIR}/Public/include/jde/crypto )
target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${sources}
        ${headers}
)

#######################################################
if(MSVC)
	find_package(fmt)
	target_link_libraries( ${CMAKE_PROJECT_NAME} fmt::fmt )
else()
	add_definitions( -DJde_Crypto_EXPORTS )
#	target_link_libraries( ${CMAKE_PROJECT_NAME} Threads::Threads )
#	target_link_libraries( ${CMAKE_PROJECT_NAME} debug libfmtd.a optimized libfmt.a )
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

  <jde/framework.h>
)

if( MSVC )
	set( CMAKE_INSTALL_PREFIX $ENV{JDE_DIR}/installed )
	set( installDir ${CMAKE_INSTALL_PREFIX}/$<CONFIGURATION> )
	set( configStem jdeCryptoConfig )
	install( TARGETS ${CMAKE_PROJECT_NAME}
		EXPORT ${configStem}
		RUNTIME DESTINATION ${installDir}/bin
		ARCHIVE DESTINATION ${installDir}/lib )
	install( FILES $<TARGET_PDB_FILE:${CMAKE_PROJECT_NAME}> CONFIGURATIONS Debug RelWithDebInfo DESTINATION ${installDir}/bin )
	export( TARGETS ${CMAKE_PROJECT_NAME} NAMESPACE Jde:: FILE ${CMAKE_CURRENT_BINARY_DIR}/${configStem}.cmake )
	install( EXPORT ${configStem} DESTINATION ${CMAKE_INSTALL_PREFIX}/cmake NAMESPACE Jde:: )
endif()