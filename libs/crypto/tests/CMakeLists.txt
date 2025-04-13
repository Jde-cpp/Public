cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( ../../../build/functions.cmake )
project( Jde.Crypto.Tests VERSION ${JDE_VERSION} LANGUAGES CXX )
include( ../../../build/dependencies.cmake )

find_package( GTest REQUIRED )

if( CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR )
	add_subdirectory( $ENV{JDE_DIR}/Framework/source framework )
endif()

link_directories( ${JSONNET_DIR}/lib )

set( targetName Jde.Crypto.Tests )
add_executable( ${targetName} )

file( GLOB sources *.cpp )
file( GLOB_RECURSE headers  *.h )
target_sources( ${targetName} PRIVATE ${sources} ${headers} ${protos} )

target_link_libraries( ${targetName} libssl.a libcrypto.a )
target_link_libraries( ${targetName} Boost::json )
target_link_libraries( ${targetName} jsonnet )
target_link_libraries( ${targetName} GTest::gtest GTest::gtest_main )
target_link_libraries( ${targetName} fmt::fmt )
target_link_libraries( ${targetName} Jde.Crypto Jde )

gtest_discover_tests( ${targetName} )
set( CMAKE_CTEST_ARGUMENTS "-c" "-settings=${CMAKE_CURRENT_SOURCE_DIR}/config/Crypto.Tests.jsonnet" )
add_test( NAME CryptoTests COMMAND $<TARGET_FILE:Jde.Crypto.Tests> )

target_precompile_headers( ${targetName} PRIVATE
	<openssl/engine.h>
	<openssl/hmac.h>
	<openssl/err.h>
	<openssl/evp.h>
	<openssl/rsa.h>
	<gtest/gtest.h>
	<jde/framework.h>
)