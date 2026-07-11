cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( ${CMAKE_CURRENT_LIST_DIR}/../../../../../build/functions.cmake )
project( Jde.OpcServer.Sqlite VERSION ${JDE_VERSION} LANGUAGES CXX )
message( ${PROJECT_NAME} )
include( ${jdeRoot}/build/dependencies.cmake )

list( APPEND CMAKE_PREFIX_PATH ${MULTI_INSTALL_PREFIX}/sqlite ) #static lib built from the amalgamation - see libs/db/drivers/sqlite/sketch.md.
find_package( SQLite3 REQUIRED )
find_package( Threads REQUIRED )

set( targetName Jde.OpcServer.Sqlite )
add_library( ${targetName} MODULE ) #dlopen'd for sqlite_api.h's RegisterAppServerProcs - never linked.
compileOptions( ${targetName} )

set_property( TARGET ${targetName} PROPERTY POSITION_INDEPENDENT_CODE ON )
file( GLOB sources *.cpp )
file( GLOB headers *.h )
target_sources( ${targetName} PRIVATE ${sources} ${headers} )

target_link_libraries( ${targetName} PRIVATE Jde.DB.Sqlite ) #RegisterProc/ExecuteStatement/ScalarUInt - the driver is SHARED so one registry is shared with it.
target_link_libraries( ${targetName} PRIVATE SQLite::SQLite3 Threads::Threads )
target_link_libraries( ${targetName} PRIVATE fmt::fmt )
target_link_libraries( ${targetName} PRIVATE Jde.DB )

target_precompile_headers( ${targetName}
  PRIVATE
	<sqlite3.h>
	<jde/fwk.h>
	<jde/fwk/str.h>
	<jde/fwk/io/json.h>
	<jde/fwk/chrono.h>
)
