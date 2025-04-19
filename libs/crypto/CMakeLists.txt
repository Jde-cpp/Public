cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
enable_testing()
include( ../../build/functions.cmake )

project( Jde.Crypto VERSION ${JDE_VERSION} LANGUAGES CXX )

add_subdirectory( src lib )
if( buildTests OR CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR )
	add_subdirectory( tests )
endif()
