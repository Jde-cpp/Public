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
target_link_libraries( ${targetName} open62541 )

target_sources( ${targetName}
    PRIVATE
        ${sources}
        ${headers}
        ${protos})

target_precompile_headers(${targetName}
  PRIVATE
		<boost/asio.hpp>
		<boost/beast.hpp>
		<boost/asio/strand.hpp>
		<boost/beast/core.hpp>
		<boost/beast/http.hpp>
		<boost/beast/ssl.hpp>
		<boost/beast/ssl/ssl_stream.hpp>
		<boost/lexical_cast.hpp>
		<boost/unordered/concurrent_flat_map.hpp>
		<boost/unordered/concurrent_flat_set.hpp>
		<boost/uuid/uuid.hpp>
		<boost/uuid/uuid_io.hpp>
		<open62541/client_config_default.h>
		<open62541/client_highlevel.h>
		<open62541/client_highlevel_async.h>
		<open62541/client_subscriptions.h>
		<open62541/plugin/log_stdout.h>
		<jde/framework.h>
		<jde/framework/str.h>
		<jde/framework/coroutine/Task.h>
		"../../../../Framework/source/DateTime.h"
		"../../../../Framework/source/coroutine/Alarm.h"
		"../../../../Framework/source/coroutine/Awaitable.h"
		<jde/crypto/OpenSsl.h>
		<jde/db/meta/AppSchema.h>
		<jde/db/meta/View.h>
		<jde/web/client/exports.h>
		<jde/web/client/proto/Web.FromServer.pb.h>
		<jde/app/shared/exports.h>
		<jde/app/shared/proto/App.FromClient.pb.h>
		<jde/app/shared/proto/App.FromClient.h>
		<jde/app/shared/proto/Common.pb.h>
		<jde/app/shared/proto/App.FromServer.pb.h>
		<jde/app/shared/proto/App.FromServer.h>
		<jde/app/shared/proto/App.pb.h>
		<jde/opc/usings.h>
		<jde/opc/exports.h>
		types/proto/Opc.Common.pb.h
		types/proto/Opc.FromServer.pb.h
		types/proto/Opc.FromClient.pb.h
		<jde/opc/uatypes/helpers.h>
		<jde/opc/uatypes/UAException.h>
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