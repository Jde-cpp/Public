cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( ${CMAKE_CURRENT_LIST_DIR}/../../../../../build/functions.cmake )

set( targetName Jde.DB.MySql.Tests )
project( ${targetName} VERSION ${JDE_VERSION} LANGUAGES CXX )
include( ${jdeRoot}/build/dependencies.cmake )
message( ${targetName} )

find_package( GTest REQUIRED )
find_package( Threads REQUIRED )

if( NOT WIN32 )
	if( NOT CMAKE_BUILD_TYPE STREQUAL "Debug" )
		set( jsonnet_DIR ${CMAKE_INSTALL_PREFIX}/jsonnet CACHE PATH "Jsonnet directory" )
		link_directories( ${jsonnet_DIR}/lib )
	endif()
endif()

add_executable( ${targetName} )
compileOptions( ${targetName} )
file( GLOB sources *.cpp )
target_sources( ${targetName} PRIVATE ${sources} ${CMAKE_CURRENT_LIST_DIR}/../src/MySqlStatements.cpp )
target_compile_definitions( ${targetName} PRIVATE TESTS )
#MySqlSyntax is header-only in ../src and MySqlStatements.cpp is a pure string builder (compiled in above), so these
#link no driver and open no connection - pure dialect/statement-text assertions.
target_link_libraries( ${targetName} GTest::gtest Threads::Threads )
target_link_libraries( ${targetName} Jde.DB Jde )

if( WIN32 )
	set_target_properties( ${targetName} PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "-tests -settings=${CMAKE_CURRENT_SOURCE_DIR}/config/MySql.Tests.jsonnet" )
	copyCommonDlls()
else()
	if( NOT CMAKE_BUILD_TYPE STREQUAL "Debug" )
		target_link_libraries( ${targetName} jsonnet++ libjsonnet.a ) #jsonnet
	endif()
endif()

addJdeTest( ${targetName} ${CMAKE_CURRENT_SOURCE_DIR}/config/MySql.Tests.jsonnet )

target_precompile_headers( ${targetName} PRIVATE pc.h )
