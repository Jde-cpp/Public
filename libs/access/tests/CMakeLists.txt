cmake_minimum_required(VERSION 3.20.0)
set( CMAKE_MODULE_PATH $ENV{JDE_DIR}/Public/build )
include(common)
findBoost()
if( ${VCPKG} )
	SET( CMAKE_TOOLCHAIN_FILE "$ENV{REPO_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"  CACHE STRING "Vcpkg toolchain file" )
endif()
project( Jde.Access.Tests )
include( compiler )
include( tests )

if( MSVC )
else()
	link_directories( ${fmt_DIR}/../.. )
	link_directories( ${Protobuf_DIR}/../.. )
	link_directories( ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} )
	link_directories( $ENV{REPO_DIR}/install/g++-13/asan/jsonnet/lib/ )
endif()
#######################################################
add_executable( ${CMAKE_PROJECT_NAME} )

file(GLOB sources *.cpp )
file(GLOB headers "**/*.h")
target_sources(${CMAKE_PROJECT_NAME}
	PRIVATE
		${sources}
		${headers}
)
#######################################################
if( MSVC )
else()
	target_link_libraries( ${CMAKE_PROJECT_NAME} Threads::Threads )
	target_link_libraries( ${CMAKE_PROJECT_NAME} debug libfmtd.a optimized libfmt.a )
	target_link_libraries( ${CMAKE_PROJECT_NAME} Boost::json )
	target_link_libraries( ${CMAKE_PROJECT_NAME} jsonnet++ )
endif()
target_link_libraries( ${CMAKE_PROJECT_NAME} gtest_main )
target_link_libraries( ${CMAKE_PROJECT_NAME} Jde Jde.Access Jde.QL Jde.DB )
#######################################################
target_precompile_headers(${CMAKE_PROJECT_NAME}
  PRIVATE
		"pc.h"
)
