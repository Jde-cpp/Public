cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
enable_testing()
include( ../../build/functions.cmake )

project( Jde.Web VERSION ${JDE_VERSION} LANGUAGES CXX )

add_subdirectory( client client )
add_subdirectory( server server )
if( buildTests OR CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR )
	add_subdirectory( tests tests )
endif()