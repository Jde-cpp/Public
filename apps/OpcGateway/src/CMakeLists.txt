cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( ${CMAKE_CURRENT_LIST_DIR}/../../../build/functions.cmake )
message( "Jde.Opc.GatewayLib" )
project( Jde.Opc.GatewayLib VERSION ${JDE_VERSION} LANGUAGES CXX )
include( ${jdeRoot}/build/dependencies.cmake )

include_directories( ${open62541Dir}include )

set( targetName Jde.Opc.GatewayLib )
add_library( ${targetName} )
compileOptions( ${targetName} )
file( GLOB sources *.cpp async/*.cpp auth/*.cpp ql/*.cpp types/*.cpp types/proto/*.cpp uatypes/*.cpp )
file( GLOB protos types/proto/*.proto )
get_filename_component( main_cpp ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp ABSOLUTE )
list( REMOVE_ITEM sources ${main_cpp} )
file( GLOB headers **/*.h )
target_sources( ${targetName} PRIVATE ${sources} ${headers} ${protos} )

IF( WIN32 )
	set_target_properties( ${targetName} PROPERTIES LINK_FLAGS "${CMAKE_EXE_LINKER_FLAGS}" )
endif()
target_include_directories( ${targetName} PRIVATE ${jdeRoot}/include/jde/app/proto )
target_link_libraries( ${targetName} Jde.Web.Server Jde.App.Client Jde.Opc )

target_precompile_headers(${targetName}
	PRIVATE
	pc.h
)

set( PROTO_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR} )
set( protoOutDir ${PROJECT_SOURCE_DIR}/types/proto )
set( protoIncludeDir "${jdeRoot}/libs/app/shared/proto" )
protobuf_generate(
	TARGET ${targetName}
	IMPORT_DIRS
		"types/proto"
		"${protoIncludeDir}"
	PROTOC_OUT_DIR ${protoOutDir})

suppressProtoWarnings( "${protos}" ${protoOutDir} )

#for intellisense; created once at configure time since recreating it as a PRE_BUILD/PRE_LINK step on every build
#bumps its mtime, which is newer than the PCH's, forcing a full PCH rebuild (and thus a full target rebuild) every time
if( NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/types/proto/Common.pb.h" )
	file( CREATE_LINK "${jdeRoot}/include/jde/app/proto/Common.pb.h" "${CMAKE_CURRENT_SOURCE_DIR}/types/proto/Common.pb.h" SYMBOLIC )
endif()