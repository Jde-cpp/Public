cmake_minimum_required(VERSION 3.20.0)
set( CMAKE_MODULE_PATH $ENV{JDE_DIR}/Public/build )
include(common)
findBoost()
project( Jde.MySql )
include(compiler)

#######################################################
link_directories( ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} )

include_directories( "${cmakeLibraryPath}/mysql-connector-cpp/include/mysqlx" )
link_directories( debug ${cmakeLibraryPath}/mysql-connector-cpp/lib64/debug optimized ${cmakeLibraryPath}/mysql-connector-cpp/lib64/release )

add_library( ${CMAKE_PROJECT_NAME} SHARED )

file(GLOB sources *.cpp )
file(GLOB headers *.h )
target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${sources}
        ${headers}
)

#######################################################

include_directories( "../../Public" )
#target_link_libraries( ${CMAKE_PROJECT_NAME} )
target_link_libraries( ${CMAKE_PROJECT_NAME} mysqlcppconn8 )
target_link_libraries( ${CMAKE_PROJECT_NAME} debug libfmtd.a optimized fmt::libfmt.a )
#######################################################
target_precompile_headers( ${CMAKE_PROJECT_NAME}
  PRIVATE
	<xdevapi.h>
	<jde/framework.h>
	<jde/framework/str.h>
	<jde/framework/io/json.h>
	"../../../../../Framework/source/DateTime.h"
)
