cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( ${CMAKE_CURRENT_LIST_DIR}/../../../../../build/functions.cmake )
include( ${jdeRoot}/build/linkSo.cmake )

set( targetName Jde.DB.Sqlite.Tests )
project( ${targetName} VERSION ${JDE_VERSION} LANGUAGES CXX )
include( ${jdeRoot}/build/dependencies.cmake )
message( ${targetName} )

list( APPEND CMAKE_PREFIX_PATH ${MULTI_INSTALL_PREFIX}/sqlite ) #static lib built from the amalgamation - see ../sketch.md; system libsqlite3-dev also works.
find_package( SQLite3 REQUIRED )
find_package( GTest REQUIRED )
find_package( Threads REQUIRED )

if( NOT WIN32 )
	if( NOT CMAKE_BUILD_TYPE STREQUAL "Debug" )
		set( jsonnet_DIR ${MULTI_INSTALL_PREFIX}/jsonnet CACHE PATH "Jsonnet directory" )
		link_directories( ${jsonnet_DIR}/lib )
	endif()
endif()

add_executable( ${targetName} )
compileOptions( ${targetName} )
file( GLOB sources *.cpp )
target_sources( ${targetName} PRIVATE ${sources} )
target_compile_definitions( ${targetName} PRIVATE TESTS JDE_SQLITE_EXPORTS )
add_dependencies( ${targetName} Jde.DB.Sqlite Jde.DB.Sqlite.AppServer Jde.DB.Sqlite.OpcServer Jde.DB.Sqlite.OpcGateway )
if( Boost_USE_STATIC_LIBS )
	target_link_libraries( ${targetName} Boost::json )
endif()
target_link_libraries( ${targetName} GTest::gtest SQLite::SQLite3 Threads::Threads )
target_link_libraries( ${targetName} Jde.Access Jde.DB Jde )

if( NOT WIN32 )
	if( NOT CMAKE_BUILD_TYPE STREQUAL "Debug" )
		target_link_libraries( ${targetName} jsonnet++ libjsonnet.a ) #jsonnet
		linkRepo( ${CMAKE_BINARY_DIR}/.. ${CMAKE_CURRENT_LIST_DIR} )
	endif()
endif()

add_test( NAME SqliteTests COMMAND $<TARGET_FILE:${targetName}> -tests -settings=${CMAKE_CURRENT_SOURCE_DIR}/config/Sqlite.Tests.jsonnet )

target_precompile_headers( ${targetName} PRIVATE pc.h )
