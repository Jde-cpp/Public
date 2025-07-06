cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( $ENV{JDE_BASH}/Public/build/functions.cmake )
message( "Jde.Opc.GatewayLib" )
project( Jde.Opc.GatewayLib VERSION ${JDE_VERSION} LANGUAGES CXX )
include( ../../../build/dependencies.cmake )

include_directories( ${open62541Dir}include )

set( targetName Jde.Opc.GatewayLib )
add_library( ${targetName} )
compileOptions( ${targetName} )
file( GLOB sources *.cpp types/proto/*.cpp )
get_filename_component( main_cpp ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp ABSOLUTE )
list( REMOVE_ITEM sources ${main_cpp} )
file( GLOB headers **/*.h )
target_sources( ${targetName} PRIVATE ${sources} ${resources} ${headers} )

target_link_libraries( ${targetName} Jde.Web.Server Jde.App.Client Jde.Opc )
#target_link_libraries( ${targetName} protobuf::libprotobuf )

target_precompile_headers(${targetName}
	PRIVATE
	pc.h
)