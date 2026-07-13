cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( ${CMAKE_CURRENT_LIST_DIR}/../../../../../build/functions.cmake )
project( Jde.DB.Sqlite.AppServer VERSION ${JDE_VERSION} LANGUAGES CXX )
message( ${PROJECT_NAME} )
include( ${jdeRoot}/build/dependencies.cmake )

sqliteProcModule( ${PROJECT_NAME} ${jdeRoot}/libs/access/config/sql/sqlite ) #AppServer hosts the access library, so its access procs are registered here too.
