cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
enable_testing()
include( ${CMAKE_CURRENT_LIST_DIR}/../../../build/functions.cmake )
project( Jde.App.Server.Tests VERSION ${JDE_VERSION} LANGUAGES CXX )
include( ${jdeRoot}/build/dependencies.cmake )

if( WIN32 )
	add_definitions( -DZLIB_USE_STATIC_LIBS )
	set( ZLIB_ROOT ${CMAKE_INSTALL_PREFIX}/zlib )
else()
	if( NOT CMAKE_BUILD_TYPE STREQUAL "Debug" )
		link_directories( ${JSONNET_DIR}/lib )
	endif()
endif()
find_package( ZLIB REQUIRED )
find_package( GTest REQUIRED )

set( targetName Jde.App.Server.Tests )
add_executable( ${targetName} )
compileOptions( ${targetName} )

file( GLOB sources *.cpp )
file( GLOB_RECURSE headers *.h )
target_sources( ${targetName} PRIVATE ${sources} ${headers} )

add_dependencies( ${targetName} Jde.DB.MySql ) #built on every platform; odbc only exists on windows.
if( WIN32 )
	add_dependencies( ${targetName} Jde.DB.Odbc )
endif()
sqliteProcDependencies( ${targetName} Jde.DB.Sqlite.AppServer ) #the ctest run below dlopens these via config/args/sqlite

if( NOT jde_Superbuild ) #superbuild already added apps/AppServer/src at the root.
	add_subdirectory( ../src ../../../apps/AppServer/lib )
endif()

target_link_libraries( ${targetName} GTest::gtest )
target_link_libraries( ${targetName} Jde.App.ServerLib )
if( NOT WIN32 AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug" )
	target_link_libraries( ${targetName} libjsonnet++.so libjsonnet.so )
endif()

addJdeTest( ${targetName} ${CMAKE_CURRENT_SOURCE_DIR}/config/App.Server.Tests.jsonnet )

if( WIN32 )
	set_target_properties( ${targetName} PROPERTIES
		VS_DEBUGGER_COMMAND_ARGUMENTS "-tests -settings=${CMAKE_CURRENT_SOURCE_DIR}/config/App.Server.Tests.jsonnet"
		RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${targetName}"
	)
	copyCommonDlls()
endif()

target_precompile_headers( ${targetName} PRIVATE pc.h )
