cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( ${CMAKE_CURRENT_LIST_DIR}/../../../../../build/functions.cmake )

set( targetName Jde.DB.Sqlite.Tests )
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
target_sources( ${targetName} PRIVATE ${sources} )
target_compile_definitions( ${targetName} PRIVATE TESTS JDE_SQLITE_EXPORTS )
sqliteProcDependencies( ${targetName} Jde.DB.Sqlite.AppServer Jde.DB.Sqlite.OpcServer Jde.DB.Sqlite.OpcGateway )
target_link_libraries( ${targetName} GTest::gtest Threads::Threads )
target_link_libraries( ${targetName} Jde.Access Jde.DB Jde )

if( NOT WIN32 )
	if( NOT CMAKE_BUILD_TYPE STREQUAL "Debug" )
		target_link_libraries( ${targetName} jsonnet++ libjsonnet.a ) #jsonnet
	endif()
endif()

addJdeTest( ${targetName} ${CMAKE_CURRENT_SOURCE_DIR}/config/Sqlite.Tests.jsonnet )

target_precompile_headers( ${targetName} PRIVATE pc.h )