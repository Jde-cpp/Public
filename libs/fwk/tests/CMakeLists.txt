cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
enable_testing()
include( $ENV{JDE_BASH}/build/functions.cmake )
include( $ENV{JDE_BASH}/build/linkSo.cmake )
project( Jde.Framework.Tests VERSION ${JDE_VERSION} LANGUAGES CXX )
include( $ENV{JDE_BASH}/build/dependencies.cmake )
message( Framework.Tests )
find_package( GTest REQUIRED )

if( NOT WIN32 )
	if( NOT CMAKE_BUILD_TYPE STREQUAL "Debug" )
		set( jsonnet_DIR ${MULTI_INSTALL_PREFIX}/jsonnet CACHE PATH "Jsonnet directory" )
		link_directories( ${jsonnet_DIR}/lib )
	endif()
endif()

set( targetName Jde.Fwk.Tests )
add_executable( ${targetName} )
compileOptions( ${targetName} )

file( GLOB sources *.cpp log/*.cpp crypto/*.cpp )
file( GLOB_RECURSE headers *.h log/*.h )
target_sources( ${targetName} PRIVATE ${sources} ${headers} )

target_link_libraries( ${targetName} GTest::gtest GTest::gtest_main )
target_link_libraries( ${targetName} Jde )
if( WIN32 )
	include_directories( ${OPENSSL_ROOT_DIR}/include )
	set_target_properties( ${targetName} PROPERTIES LINK_FLAGS "${CMAKE_EXE_LINKER_FLAGS}" )
else()
	if( NOT CMAKE_BUILD_TYPE STREQUAL "Debug" )
		message( adding jsonnet++ and libjsonnet.a to link libraries )
		target_link_libraries( ${targetName} jsonnet++ libjsonnet.a ) #jsonnet
	endif()
endif()

add_test( NAME FwkTests COMMAND $<TARGET_FILE:Jde.Fwk.Tests> -tests -settings=$ENV{JDE_DIR}/libs/fwk/tests/config/Framework.Tests.jsonnet )
set( CMAKE_CTEST_ARGUMENTS "-tests;-settings=${CMAKE_CURRENT_SOURCE_DIR}/config/Framework.Tests.jsonnet" )


target_precompile_headers( ${targetName} PRIVATE pc.h )
if( WIN32 )
	set_target_properties( ${targetName} PROPERTIES
		VS_DEBUGGER_COMMAND_ARGUMENTS "-settings=${CMAKE_CURRENT_SOURCE_DIR}/config/Framework.Tests.jsonnet"
	)
	copyLibDlls()
else()
	if( NOT CMAKE_BUILD_TYPE STREQUAL "Debug" )
		linkRepo( ${CMAKE_BINARY_DIR}/lib ${CMAKE_CURRENT_LIST_DIR} )
	endif()
endif()