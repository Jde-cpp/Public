cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( $ENV{JDE_DIR}/build/functions.cmake )
project( Jde.Opc.Server VERSION ${JDE_VERSION} LANGUAGES CXX )

include( ../../../build/dependencies.cmake )

find_package( open62541 REQUIRED )
find_package( NodesetLoader REQUIRED )
if( WIN32 )
	if( NOT jde_REPOS )
		set( LIBXML2_LIBRARY ${MULTI_INSTALL_PREFIX}/libxml2/lib/libxml2$<IF:$<CONFIG:Debug>,d,>.lib CACHE PATH "the directory containing LibXml2 libraries" )
		set( LIBXML2_INCLUDE_DIR ${MULTI_INSTALL_PREFIX}/libxml2/include/libxml2 CACHE PATH "the directory containing LibXml2 headers" )
	endif()
	find_package( LibXml2 REQUIRED )
endif()
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

target_link_libraries( ${targetName} PUBLIC Jde.App.Client Jde.Web.Server Jde.Opc )
IF( WIN32 )
	message( "LIBXML2_LIBRARY=${LIBXML2_LIBRARY}" )
	target_link_libraries( ${targetName} PUBLIC ${LIBXML2_LIBRARY} )
endif()
target_link_libraries( ${targetName} PUBLIC NodesetLoader )
#######################################################
target_precompile_headers( ${targetName}
	PRIVATE
		pc.h
)
add_custom_command( TARGET ${targetName} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink "$ENV{JDE_DIR}/libs/db/config/common-meta.libsonnet" "$ENV{JDE_DIR}/apps/OpcServer/config/common-meta.libsonnet" COMMENT "mklink common-meta.libsonnet" )
