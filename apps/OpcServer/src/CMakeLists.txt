cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( $ENV{JDE_DIR}/Public/build/functions.cmake )
project( Jde.Opc.Server VERSION ${JDE_VERSION} LANGUAGES CXX )

include( ../../../build/dependencies.cmake )

message( CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH} )
find_package( open62541 REQUIRED )
find_package( NodesetLoader REQUIRED )
set( open62541Dir ${open62541_DIR}/../../.. )
#cmake_path( ABSOLUTE_PATH open62541Dir NORMALIZE OUTPUT_VARIABLE open62541Dir )
#NodesetLoader_DIR=/home/duffyj/code/libs/install/g++-14/Debug/NodesetLoader/lib/cmake/NodesetLoader
#dumpVariables()

set( targetName Jde.Opc.ServerLib )
add_library( ${targetName} )
compileOptions( ${targetName} )
file(GLOB sources *.cpp **/*.cpp )
get_filename_component( main_cpp ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp ABSOLUTE )
list( REMOVE_ITEM sources ${main_cpp} )
file(GLOB headers "**/*.h")
target_sources( "${targetName}" PRIVATE "${sources}" "${headers}" )

IF( WIN32 )
	set_target_properties( ${targetName} PROPERTIES LINK_FLAGS ${CMAKE_EXE_LINKER_FLAGS} )
else()
  target_include_directories( ${targetName} PRIVATE /usr/include/libxml2 )
endif()

include_directories( ${open62541Dir}/include )

target_link_libraries( ${targetName} Jde.App.Client Jde.Web.Server Jde.Opc )
target_link_libraries( ${targetName} NodesetLoader )
#######################################################
target_precompile_headers( ${targetName}
	PRIVATE
		pc.h
)
add_custom_command( TARGET ${targetName} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink "$ENV{JDE_DIR}/Public/libs/db/config/common-meta.libsonnet" "$ENV{JDE_DIR}/Public/apps/OpcServer/config/common-meta.libsonnet" COMMENT "mklink common-meta.libsonnet" )
