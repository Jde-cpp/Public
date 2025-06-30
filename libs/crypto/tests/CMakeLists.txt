#./Jde.Crypto.Tests.exe -c -settings=/C/Users/duffyj/source/repos/jde/Public/libs/crypto/tests/config/Crypto.Tests.jsonnet
cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( ../../../build/functions.cmake )
project( Jde.Crypto.Tests VERSION ${JDE_VERSION} LANGUAGES CXX )
include( ../../../build/dependencies.cmake )

find_package( GTest REQUIRED )

if( WIN32 )
	link_directories( ${OPENSSL_CRYPTO_LIBRARY_DIR} )
endif()

set( targetName Jde.Crypto.Tests )
add_executable( ${targetName} )
compileOptions( ${targetName} )
if( WIN32)
	set_target_properties( ${targetName} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>" )
endif()


file( GLOB sources *.cpp )
file( GLOB_RECURSE headers  *.h )
target_sources( ${targetName} PRIVATE ${sources} ${headers} ${protos} )

target_link_libraries( ${targetName} Boost::json )
target_link_libraries( ${targetName} GTest::gtest GTest::gtest_main )
target_link_libraries( ${targetName} fmt::fmt )
target_link_libraries( ${targetName} Jde.Crypto Jde )
if( WIN32 )
	include_directories( ${OPENSSL_ROOT_DIR}/include )
else()
	target_link_libraries( ${targetName} libssl.a libcrypto.a )
endif()

add_test( NAME CryptoTests COMMAND $<TARGET_FILE:Jde.Crypto.Tests> )
set( CMAKE_CTEST_ARGUMENTS "-settings=${CMAKE_CURRENT_SOURCE_DIR}/config/Crypto.Tests.jsonnet" )
add_dependencies( ${targetName} Jde)

target_precompile_headers( ${targetName} PRIVATE
	<openssl/engine.h>
	<openssl/hmac.h>
	<openssl/err.h>
	<openssl/evp.h>
	<openssl/rsa.h>
	<gtest/gtest.h>
	<jde/framework.h>
)
if( WIN32 )
	set( buildLibDir ${CMAKE_BINARY_DIR}/../../libs )
	cmake_path( ABSOLUTE_PATH buildLibDir NORMALIZE OUTPUT_VARIABLE buildLibDir )
	set_target_properties( ${targetName} PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "-tests -settings=${CMAKE_CURRENT_SOURCE_DIR}/config/Crypto.Tests.jsonnet" )
	add_custom_command( TARGET ${targetName} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${buildLibDir}/framework/$<CONFIG>/Jde.dll" $<TARGET_FILE_DIR:${targetName}> )
	add_custom_command( TARGET ${targetName} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${LIB_DIR}/fmt/bin/fmtd.dll" $<TARGET_FILE_DIR:${targetName}>  COMMENT "fmtd.dll" )
	add_custom_command( TARGET ${targetName} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${LIB_DIR}/fmt/bin/fmt.dll" $<TARGET_FILE_DIR:${targetName}>  COMMENT "fmt.dll" )
endif()