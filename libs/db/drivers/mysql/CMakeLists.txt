cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
enable_testing()
include( ../../../../build/functions.cmake )
project( Jde.DB.MySql VERSION ${JDE_VERSION} LANGUAGES CXX )
include( ../../../../build/dependencies.cmake )

find_package( mysql-concpp REQUIRED )
include_directories( "${MYSQL_CONCPP_INCLUDE_DIR}" )

set( targetName Jde.DB.MySql )
add_library( ${targetName} SHARED )

set_property( TARGET ${targetName} PROPERTY POSITION_INDEPENDENT_CODE ON )
file(GLOB sources *.cpp )
file(GLOB headers *.h )
target_sources(${targetName} PRIVATE ${sources} ${headers} )

target_link_libraries( ${targetName} PRIVATE mysql::concpp )
target_link_libraries( ${targetName} PRIVATE fmt::fmt )
target_link_libraries( ${targetName} PRIVATE Jde.DB )



target_precompile_headers( ${targetName}
  PRIVATE
	<mysqlx/xdevapi.h>
	<jde/framework.h>
	<jde/framework/str.h>
	<jde/framework/io/json.h>
	../../../../../Framework/source/DateTime.h
)
