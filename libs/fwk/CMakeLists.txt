cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
enable_testing()
include( $ENV{JDE_DIR}/Public/build/functions.cmake )

project( Jde.Framework VERSION ${JDE_VERSION} LANGUAGES CXX )

add_subdirectory( src lib )
if( jde_BUILD_TESTS OR CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR )
	message( "Building tests for ${PROJECT_NAME}" )
	add_subdirectory( tests )
endif()