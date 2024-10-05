cmake_minimum_required(VERSION 3.20.0)
set( CMAKE_MODULE_PATH $ENV{JDE_DIR}/Public/build )
include(common)
findBoost()
if( ${VCPKG} )
	SET(CMAKE_TOOLCHAIN_FILE "$ENV{REPO_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"  CACHE STRING "Vcpkg toolchain file")
endif()
project( Jde.UM )
include(compiler)

#set( CMAKE_MODULE_PATH $ENV{JDE_DIR}/Public/build )
if(MSVC)
else()
	link_directories( ${fmt_DIR}/../.. )
	link_directories( ${Protobuf_DIR}/../.. )
	add_definitions( -DJde_UM_EXPORTS )
endif()
#######################################################
add_library( ${CMAKE_PROJECT_NAME} SHARED )
file(GLOB source *.cpp )
file(GLOB headers *.h)
target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${source}
        ${headers}
)
#######################################################
if(MSVC)
else()
	target_link_libraries( ${CMAKE_PROJECT_NAME} Threads::Threads )
	target_link_libraries( ${CMAKE_PROJECT_NAME} debug libfmtd.a optimized libfmt.a )
endif()
#######################################################
target_precompile_headers(${CMAKE_PROJECT_NAME}
  PRIVATE

  <jde/TypeDefs.h>
	<jde/log/Log.h>
	<jde/Exception.h>
)
