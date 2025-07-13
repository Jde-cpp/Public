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
target_sources( ${targetName} PRIVATE ${sources} ${resources} ${headers} ${protos} )

target_link_libraries( ${targetName} Jde.Web.Server Jde.App.Client Jde.Opc )
#target_link_libraries( ${targetName} protobuf::libprotobuf )

target_precompile_headers(${targetName}
	PRIVATE
	pc.h
)

set(PROTO_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})
set( protoOutDir ${PROJECT_SOURCE_DIR}/types/proto )
set( protoIncludeDir "-I $ENV{JDE_DIR}/Public/libs/app/shared/proto/Common.proto" )
protobuf_generate(
	TARGET ${targetName}
	IMPORT_DIRS types/proto
	PROTOC_OUT_DIR ${protoOutDir})

#file( MAKE_DIRECTORY $ENV{JDE_DIR}/Public/include/jde/opc/types/proto )
#add_custom_command( TARGET ${targetName} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink "${protoOutDir}/Opc.Common.pb.h" "$ENV{JDE_DIR}/Public/include/jde/opc/types/proto/Opc.Common.pb.h" COMMENT "mklink Opc.Common" )
#add_custom_command( TARGET ${targetName} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink "${protoOutDir}/Opc.FromClient.pb.h" "$ENV{JDE_DIR}/Public/include/jde/opc/types/proto/Opc.FromClient.pb.h" COMMENT "mklink Opc.FromClient" )
#add_custom_command( TARGET ${targetName} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink "${protoOutDir}/Opc.FromServer.pb.h" "$ENV{JDE_DIR}/Public/include/jde/opc/types/proto/Opc.FromServer.pb.h" COMMENT "mklink Opc.FromServer" )