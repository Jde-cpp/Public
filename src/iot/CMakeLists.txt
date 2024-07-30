cmake_minimum_required(VERSION 3.16)
project(Jde.Iot)
set( CMAKE_MODULE_PATH $ENV{JDE_DIR}/Public/build )
include(common)
link_directories( ${fmt_DIR}/../.. )
link_directories( ${Protobuf_DIR}/../.. )
#######################################################
set(CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS} "-Wl,-rpath=$ORIGIN")
add_definitions( -DJde_Iot_EXPORTS -DUA_ENABLE_ENCRYPTION )

find_package(OpenSSL REQUIRED)
find_package(open62541 REQUIRED)
set( open62541Dir ${open62541_DIR}/../../.. )
#message( "open62541Dir=${open62541Dir}")
link_directories( ${open62541Dir}/lib )
include_directories( ${open62541Dir}/include )
include_directories( $ENV{JDE_DIR}/Public/jde/app/shared/proto )

add_library( ${CMAKE_PROJECT_NAME} SHARED )
set( CMAKE_INCLUDE_CURRENT_DIR ON )
file(GLOB src_files *.cpp async/*.cpp await/*.cpp types/*.cpp uatypes/*.cpp )
file(GLOB hdr_files  **/*.h )
file(GLOB proto_files types/proto/*.proto )
get_filename_component(common_proto ${CMAKE_CURRENT_SOURCE_DIR}/types/proto/Common.proto ABSOLUTE)
message( "common_proto=${common_proto}" )
list(REMOVE_ITEM proto_files "${common_proto}")
message( "proto_files=${proto_files}" )

target_sources( "${CMAKE_PROJECT_NAME}"
    PRIVATE
        "${src_files}"
        "${hdr_files}"
        "${proto_files}")

#######################################################
#target_link_libraries( ${CMAjdeKE_PROJECT_NAME} Threads::Threads )
target_link_libraries( ${CMAKE_PROJECT_NAME} debug libfmtd.a optimized libfmt.a )
target_link_libraries(${CMAKE_PROJECT_NAME} OpenSSL::SSL)
target_link_libraries( ${CMAKE_PROJECT_NAME} protobuf::libprotobuf )
target_link_libraries( ${CMAKE_PROJECT_NAME} libopen62541.a )
#######################################################
target_precompile_headers(${CMAKE_PROJECT_NAME}
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
		<jde/iot/TypeDefs.h>
		<jde/Exports.h>
		<jde/log/Log.h>
		<jde/App.h>
		<jde/Str.h>
		<jde/coroutine/Task.h>
		<jde/crypto/OpenSsl.h>
		<jde/web/server/exports.h>
		"../../../Framework/source/DateTime.h"
		"../../../Framework/source/coroutine/Alarm.h"
		"../../../Framework/source/coroutine/Awaitable.h"
		"../../../Framework/source/db/Database.h"
		<jde/iot/Exports.h>
		<jde/app/shared/proto/App.FromClient.pb.h>
		<jde/app/shared/proto/App.FromClient.h>
		<jde/app/shared/proto/Common.pb.h>
		<jde/app/shared/proto/App.FromServer.pb.h>
		<jde/app/shared/proto/App.FromServer.h>
		<jde/app/shared/proto/App.pb.h>
		<Iot.Common.pb.h>
		<Iot.FromServer.pb.h>
		<Iot.FromClient.pb.h>
		<jde/iot/uatypes/helpers.h>
		<jde/iot/uatypes/UAException.h>
)

protobuf_generate(
	TARGET "${CMAKE_PROJECT_NAME}"
	IMPORT_DIRS types/proto . )

add_custom_command( TARGET ${CMAKE_PROJECT_NAME} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink "${CMAKE_BINARY_DIR}/Iot.Common.pb.h" "$ENV{JDE_DIR}/Public/jde/iot/types/proto/Iot.Common.pb.h" COMMENT "mklink Common" )
add_custom_command( TARGET ${CMAKE_PROJECT_NAME} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink "${CMAKE_BINARY_DIR}/Iot.FromClient.pb.h" "$ENV{JDE_DIR}/Public/jde/iot/types/proto/Iot.FromClient.pb.h" COMMENT "mklink FromClient" )
add_custom_command( TARGET ${CMAKE_PROJECT_NAME} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink "${CMAKE_BINARY_DIR}/Iot.FromServer.pb.h" "$ENV{JDE_DIR}/Public/jde/iot/types/proto/Iot.FromServer.pb.h" COMMENT "mklink FromServer" )