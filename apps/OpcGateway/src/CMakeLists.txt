cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( $ENV{JDE_BASH}/Public/build/functions.cmake )
message( "Jde.Opc.GatewayLib" )
project( Jde.Opc.GatewayLib VERSION ${JDE_VERSION} LANGUAGES CXX )
include( ../../../build/dependencies.cmake )

include_directories( ${open62541Dir}include )

set( targetName Jde.Opc.GatewayLib )
add_library( ${targetName} )
compileOptions( ${targetName} )
file( GLOB sources *.cpp async/*.cpp auth/*.cpp types/*.cpp types/proto/*.cpp uatypes/*.cpp )
file( GLOB protos types/proto/*.proto )
get_filename_component( main_cpp ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp ABSOLUTE )
list( REMOVE_ITEM sources ${main_cpp} )
file( GLOB headers **/*.h )
target_sources( ${targetName} PRIVATE ${sources} ${headers} ${protos} )

target_link_libraries( ${targetName} Jde.Web.Server Jde.App.Client Jde.Opc )

target_precompile_headers(${targetName}
	PRIVATE
	pc.h
)

set( PROTO_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR} )
set( protoOutDir ${PROJECT_SOURCE_DIR}/types/proto )
set( protoIncludeDir "$ENV{JDE_DIR}/Public/libs/app/shared/proto" )
protobuf_generate(
	TARGET ${targetName}
	IMPORT_DIRS
		"types/proto"
		"${protoIncludeDir}"
	PROTOC_OUT_DIR ${protoOutDir})
add_custom_command( TARGET ${targetName} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink "$ENV{JDE_BASH}/Public/include/jde/app/shared/proto/Common.pb.h" "${CMAKE_CURRENT_SOURCE_DIR}/types/proto/Common.pb.h" COMMENT "mklink Common.pb.h" )