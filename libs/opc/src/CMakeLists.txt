cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( ../../../build/functions.cmake )
message( Jde.Opc )
project( Jde.Opc VERSION ${JDE_VERSION} LANGUAGES CXX )
include( ../../../build/dependencies.cmake )
if( WIN32 )
else()
	find_package( OpenSSL )
endif()
find_package( open62541 REQUIRED )
set( open62541Dir ${open62541_DIR}/../../.. )
cmake_path( ABSOLUTE_PATH open62541Dir NORMALIZE OUTPUT_VARIABLE open62541Dir )
link_directories( ${open62541Dir}/lib )
add_definitions( -DUA_ENABLE_ENCRYPTION )

set( targetName Jde.Opc )
add_library( ${targetName} )
compileOptions( ${targetName} )
file( GLOB_RECURSE sources *.cpp )
file( GLOB_RECURSE headers  *.h )
file( GLOB protos types/proto/*.proto )

include_directories( ${open62541Dir}include )
if( NOT WIN32 )
	target_link_libraries( ${targetName} stdc++exp )#for stacktrace
endif()
target_link_libraries( ${targetName} protobuf::libprotobuf )
target_link_libraries( ${targetName} open62541 )

target_sources( ${targetName}
    PRIVATE
        ${sources}
        ${headers}
        ${protos})

target_precompile_headers(${targetName}
  PRIVATE
		"pc.h"
)
set(PROTO_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

set( protoOutDir ${PROJECT_SOURCE_DIR}/types/proto )
#dumpVariables()
set( protoIncludeDir "-I $ENV{JDE_DIR}/Public/libs/app/shared/proto/Common.proto" )
protobuf_generate(
	TARGET ${targetName}
	IMPORT_DIRS types/proto
	PROTOC_OUT_DIR ${protoOutDir}
	EXPORT_MACRO Jde_Opc_EXPORTS )

file( MAKE_DIRECTORY $ENV{JDE_DIR}/Public/include/jde/opc/types/proto )
add_custom_command( TARGET ${targetName} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink "${protoOutDir}/Opc.Common.pb.h" "$ENV{JDE_DIR}/Public/include/jde/opc/types/proto/Opc.Common.pb.h" COMMENT "mklink Opc.Common" )
add_custom_command( TARGET ${targetName} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink "${protoOutDir}/Opc.FromClient.pb.h" "$ENV{JDE_DIR}/Public/include/jde/opc/types/proto/Opc.FromClient.pb.h" COMMENT "mklink Opc.FromClient" )
add_custom_command( TARGET ${targetName} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink "${protoOutDir}/Opc.FromServer.pb.h" "$ENV{JDE_DIR}/Public/include/jde/opc/types/proto/Opc.FromServer.pb.h" COMMENT "mklink Opc.FromServer" )