cmake_minimum_required(VERSION 3.20)
set( CMAKE_MODULE_PATH $ENV{JDE_DIR}/Public/build )
include(common)
findBoost()
if( ${VCPKG} )
	SET( CMAKE_TOOLCHAIN_FILE $ENV{REPO_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake  CACHE STRING "Vcpkg toolchain file" )
endif()
project( Jde.Iot )
include( compiler )
find_package( OpenSSL )
find_package( open62541 REQUIRED )
#dump_cmake_variables()
if( MSVC )
	find_package( Protobuf REQUIRED )
	find_package(nlohmann_json REQUIRED)
	include_directories( ${OPENSSL_INCLUDE_DIR} )
	set( installDir $ENV{REPO_DIR}/installed/$(ConfigurationName) )
	include_directories( ${installDir}/include )
	link_directories( ${installDir}/libs )
	link_directories( $ENV{JDE_DIR}/Framework/source/.build/.bin/$(ConfigurationName) )
	link_directories( $ENV{REPO_DIR}/open62541/.build/bin/$(ConfigurationName) )
	link_directories( ../crypto/.build/.bin/$(ConfigurationName) )
	link_directories( ../web/server/.build/.bin/$(ConfigurationName) )
	link_directories( ../app/shared/.build/.bin/$(ConfigurationName) )
	link_directories( ../web/client/.build/.bin/$(ConfigurationName) )
	link_directories( ../app/client/.build/.bin/$(ConfigurationName) )

#	list( APPEND CMAKE_PREFIX_PATH "$ENV{REPO_DIR}" )
	#list( APPEND CMAKE_MODULE_PATH "$ENV{REPO_DIR}" )
#	set( open62541_TOOLS_DIR $ENV{REPO_DIR}/open62541/tools )
#	message( open62541_TOOLS_DIR: "${open62541_TOOLS_DIR}" )
	#message( CMAKE_MODULE_PATH: "${CMAKE_MODULE_PATH}" )
#	find_package(open62541 REQUIRED PATH_SUFFIXES .build )
else()
	link_directories( ${fmt_DIR}/../.. )
	link_directories( ${Protobuf_DIR}/../.. )
	add_definitions( -DJde_Iot_EXPORTS )
	set( open62541Dir ${open62541_DIR}/../../.. )
	link_directories( ${open62541Dir}/lib )
	include_directories( ${open62541Dir}/include )
endif()
include_directories( $ENV{JDE_DIR}/Public/jde/app/shared/proto )
add_definitions( -DUA_ENABLE_ENCRYPTION )
#######################################################
add_library( ${CMAKE_PROJECT_NAME} SHARED )
set( CMAKE_INCLUDE_CURRENT_DIR ON )

file( GLOB_RECURSE sources *.cpp )
file( GLOB_RECURSE headers  *.h )
file( GLOB protos types/proto/*.proto )
get_filename_component( common_proto ${CMAKE_CURRENT_SOURCE_DIR}/types/proto/Common.proto ABSOLUTE )
#message( "common_proto=${common_proto}" )
list( REMOVE_ITEM protos ${common_proto} )
#message( "proto_files=${protos}" )

target_sources( ${CMAKE_PROJECT_NAME}
    PRIVATE
        ${sources}
        ${headers}
        ${protos})

#######################################################
if(MSVC)
	find_package( fmt REQUIRED )
	find_package( ZLIB )

	target_link_libraries( ${CMAKE_PROJECT_NAME} fmt::fmt )
	target_link_libraries( ${CMAKE_PROJECT_NAME} ZLIB::ZLIB )
	target_link_libraries( ${CMAKE_PROJECT_NAME} open62541.lib )
	target_link_libraries( ${CMAKE_PROJECT_NAME} nlohmann_json::nlohmann_json )

	string( REPLACE "/W4" "/W0" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG} )
	set_source_files_properties( ${sources} PROPERTIES COMPILE_FLAGS "/W4" )
else()
	#target_link_libraries( ${CMAjdeKE_PROJECT_NAME} Threads::Threads )
	target_link_libraries( ${CMAKE_PROJECT_NAME} debug libfmtd.a optimized libfmt.a )
	target_link_libraries( ${CMAKE_PROJECT_NAME} libopen62541.a )
endif()
if( WIN32 )
  target_link_libraries( ${CMAKE_PROJECT_NAME} wsock32 ws2_32 )
	target_link_libraries( ${CMAKE_PROJECT_NAME} Iphlpapi )
endif()
target_link_libraries( ${CMAKE_PROJECT_NAME} OpenSSL::SSL )
target_link_libraries( ${CMAKE_PROJECT_NAME} protobuf::libprotobuf )

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
		<jde/app/shared/exports.h>
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
set(PROTO_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

protobuf_generate( TARGET ${CMAKE_PROJECT_NAME} IMPORT_DIRS types/proto . EXPORT_MACRO Jde_Iot_EXPORTS )

file( MAKE_DIRECTORY $ENV{JDE_DIR}/Public/jde/iot/types/proto )
add_custom_command( TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink "${CMAKE_BINARY_DIR}/Iot.Common.pb.h" "$ENV{JDE_DIR}/Public/jde/iot/types/proto/Iot.Common.pb.h" COMMENT "mklink Common" )
add_custom_command( TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink "${CMAKE_BINARY_DIR}/Iot.FromClient.pb.h" "$ENV{JDE_DIR}/Public/jde/iot/types/proto/Iot.FromClient.pb.h" COMMENT "mklink FromClient" )
add_custom_command( TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink "${CMAKE_BINARY_DIR}/Iot.FromServer.pb.h" "$ENV{JDE_DIR}/Public/jde/iot/types/proto/Iot.FromServer.pb.h" COMMENT "mklink FromServer" )

if( MSVC )
	set( CMAKE_INSTALL_PREFIX $ENV{JDE_DIR}/installed )
	set( installDir ${CMAKE_INSTALL_PREFIX}/$<CONFIGURATION> )
	set( configStem jdeIotConfig )
	install( TARGETS ${CMAKE_PROJECT_NAME}
		EXPORT ${configStem}
		RUNTIME DESTINATION ${installDir}/bin
		ARCHIVE DESTINATION ${installDir}/lib )
	install( FILES $<TARGET_PDB_FILE:${CMAKE_PROJECT_NAME}> CONFIGURATIONS Debug RelWithDebInfo DESTINATION ${installDir}/bin )
#/home/duffyj/code/libraries/install/g++-13/asan/spdlog/lib/cmake/spdlog/
	export( TARGETS ${CMAKE_PROJECT_NAME} NAMESPACE Jde:: FILE ${CMAKE_CURRENT_BINARY_DIR}/${configStem}.cmake )
	install( EXPORT ${configStem} DESTINATION ${CMAKE_INSTALL_PREFIX}/cmake NAMESPACE Jde:: )
	#install( FILES "${CMAKE_CURRENT_BINARY_DIR}/${exportFile}.cmake" DESTINATION ${installDir}/lib/cmake/${CMAKE_PROJECT_NAME} )
endif()