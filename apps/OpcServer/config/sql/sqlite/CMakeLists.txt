cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( ${CMAKE_CURRENT_LIST_DIR}/../../../../../build/functions.cmake )
project( Jde.DB.Sqlite.OpcServer VERSION ${JDE_VERSION} LANGUAGES CXX )
message( ${PROJECT_NAME} )
include( ${jdeRoot}/build/dependencies.cmake )

list( APPEND CMAKE_PREFIX_PATH ${MULTI_INSTALL_PREFIX}/sqlite ) #static lib built from the amalgamation - see libs/db/drivers/sqlite/sketch.md.
find_package( SQLite3 REQUIRED )
find_package( Threads REQUIRED )

set( targetName ${PROJECT_NAME} )
add_library( ${targetName} MODULE ) #dlopen'd for sqlite_api.h's RegisterProcs( IProcs& ) - never linked.
compileOptions( ${targetName} )

set_property( TARGET ${targetName} PROPERTY POSITION_INDEPENDENT_CODE ON )
file( GLOB sources *.cpp )
file( GLOB headers *.h )
target_sources( ${targetName} PRIVATE ${sources} ${headers} )

target_link_libraries( ${targetName} PRIVATE SQLite::SQLite3 Threads::Threads ) #sqlite3 C API used directly (last_insert_rowid); the driver is reached only through IProcs at runtime.
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
