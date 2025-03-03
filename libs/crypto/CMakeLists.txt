cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
enable_testing()
include( ../../build/functions.cmake )

project( Jde.Crypto VERSION ${JDE_VERSION} LANGUAGES CXX )

add_subdirectory( $ENV{JDE_DIR}/Framework/source framework )
add_subdirectory( src )
if( buildTests OR CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR )
	add_subdirectory( tests )
	set( CMAKE_CTEST_ARGUMENTS "-c" )
	add_test( NAME CryptoTests COMMAND $<TARGET_FILE:Jde.Crypto.Tests> )
endif()
