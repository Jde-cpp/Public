cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
enable_testing()
include( ../../../../build/functions.cmake )
project( Jde.DB.MySql VERSION ${JDE_VERSION} LANGUAGES CXX )
include( ../../../../build/dependencies.cmake )

#message( CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH} )
find_package( mysql-concpp REQUIRED debug )
#dumpVariables()
include_directories( "${MYSQL_CONCPP_INCLUDE_DIR}" )
#link_directories( debug ${cmakeLibraryPath}/mysql-connector-cpp/lib64/debug optimized ${cmakeLibraryPath}/mysql-connector-cpp/lib64/release )

set( targetName Jde.DB.MySql )
add_library( ${targetName} SHARED )

set_property( TARGET ${targetName} PROPERTY POSITION_INDEPENDENT_CODE ON )
file(GLOB sources *.cpp )
file(GLOB headers *.h )
target_sources(${targetName} PRIVATE ${sources} ${headers} )

target_link_libraries( ${targetName} mysql::concpp )
target_link_libraries( ${targetName} fmt::fmt )
#target_link_libraries( ${targetName} debug libfmtd.a optimized fmt::libfmt.a )
target_link_libraries( ${targetName} Jde.DB )



target_precompile_headers( ${targetName}
  PRIVATE
	<mysqlx/xdevapi.h>
	<jde/framework.h>
	<jde/framework/str.h>
	<jde/framework/io/json.h>
	../../../../../Framework/source/DateTime.h
)
