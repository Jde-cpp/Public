cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( ${CMAKE_CURRENT_LIST_DIR}/../../../../../build/functions.cmake )
project( Jde.DB.Sqlite VERSION ${JDE_VERSION} LANGUAGES CXX )
message( ${PROJECT_NAME} )
include( ${jdeRoot}/build/dependencies.cmake )

list( APPEND CMAKE_PREFIX_PATH ${MULTI_INSTALL_PREFIX}/sqlite ) #static lib built from the amalgamation - see sketch.md; system libsqlite3-dev also works.
find_package( SQLite3 REQUIRED )
find_package( Threads REQUIRED )

set( targetName Jde.DB.Sqlite )
add_library( ${targetName} SHARED ) #SHARED not MODULE: still dlopen'd like the other drivers, but apps/AppServer/config/sql/sqlite links RegisterProc etc.
compileOptions( ${targetName} )
target_compile_definitions( ${targetName} PRIVATE JDE_SQLITE_EXPORTS )

set_property( TARGET ${targetName} PROPERTY POSITION_INDEPENDENT_CODE ON )
file( GLOB sources *.cpp )
file( GLOB headers *.h )
target_sources( ${targetName} PRIVATE ${sources} ${headers} )

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
	usings.h
)
