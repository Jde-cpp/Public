cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( $ENV{JDE_DIR}/Public/build/functions.cmake )
project( Jde.Opc.Server VERSION ${JDE_VERSION} LANGUAGES CXX )

include( $ENV{JDE_DIR}/Public/build/dependencies.cmake )

set( targetName Jde.Opc.ServerLib )
add_library( ${targetName} )
compileOptions( ${targetName} )
file(GLOB sources *.cpp **/*.cpp )
get_filename_component( main_cpp ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp ABSOLUTE )
list( REMOVE_ITEM sources ${main_cpp} )
file(GLOB headers "**/*.h")
target_sources( "${targetName}" PRIVATE "${sources}" "${headers}" )

include_directories( ${open62541Dir}/include )

target_link_libraries( ${targetName} Jde.App.Client Jde.Web.Server Jde.Opc )
#######################################################
target_precompile_headers( ${targetName}
	PRIVATE
		pc.h
)
