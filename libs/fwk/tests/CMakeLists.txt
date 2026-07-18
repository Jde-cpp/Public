cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
enable_testing()
include( ${CMAKE_CURRENT_LIST_DIR}/../../../build/functions.cmake )
project( Jde.Framework.Tests VERSION ${JDE_VERSION} LANGUAGES CXX )
include( ${jdeRoot}/build/dependencies.cmake )
message( Framework.Tests )
find_package( GTest REQUIRED )

set( targetName Jde.Fwk.Tests )
add_executable( ${targetName} )
compileOptions( ${targetName} )

file( GLOB sources *.cpp co/*.cpp log/*.cpp crypto/*.cpp )
file( GLOB_RECURSE headers *.h log/*.h )
target_sources( ${targetName} PRIVATE ${sources} ${headers} )

target_link_libraries( ${targetName} GTest::gtest GTest::gtest_main )
target_link_libraries( ${targetName} Jde )
if( WIN32 )
	include_directories( ${OPENSSL_ROOT_DIR}/include )
endif()

addJdeTest( ${targetName} ${CMAKE_CURRENT_SOURCE_DIR}/config/Framework.Tests.jsonnet )

target_precompile_headers( ${targetName} PRIVATE pc.h )
if( WIN32 )
	set_target_properties( ${targetName} PROPERTIES
		VS_DEBUGGER_COMMAND_ARGUMENTS "-tests -settings=${CMAKE_CURRENT_SOURCE_DIR}/config/Framework.Tests.jsonnet"
	)
	copyLibDlls()
endif()