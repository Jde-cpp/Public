cmake_minimum_required(VERSION 3.20)
set( CMAKE_MODULE_PATH $ENV{JDE_DIR}/Public/build )
include(common)
findBoost()
if( ${VCPKG} )
	SET( CMAKE_TOOLCHAIN_FILE $ENV{REPO_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake  CACHE STRING "Vcpkg toolchain file" )
endif()

project( Jde.Iot.Tests VERSION 1.0.0.0 )
include( compiler )
if( NOT ${VCPKG} )
	include( tests )
endif()
find_package( OpenSsl REQUIRED )
find_package( protobuf REQUIRED )
if( MSVC )
	include_directories( ${OPENSSL_INCLUDE_DIR} )
	set( installDir $ENV{REPO_DIR}/installed/$(ConfigurationName) )
	include_directories( ${installDir}/include )
	link_directories( ${installDir}/libs )
	link_directories( $ENV{JDE_DIR}/Framework/source/.build/.bin/$(ConfigurationName) )
	link_directories( ../../src/web/server/.build/.bin/$(ConfigurationName) )
	link_directories( ../../src/iot/.build/.bin/$(ConfigurationName) )
	link_directories( ../../src/app/shared/.build/.bin/$(ConfigurationName) )
	link_directories( ../../src/web/client/.build/.bin/$(ConfigurationName) )
	link_directories( ../../src/crypto/.build/.bin/$(ConfigurationName) )
	link_directories( ../../src/app/client/.build/.bin/$(ConfigurationName) )
else()
	link_directories( ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} )
	link_directories( ${fmt_DIR}/../.. )
	link_directories( ${Protobuf_DIR}/../.. )

	find_package(open62541 REQUIRED)
	set( open62541Dir ${open62541_DIR}/../../.. )
	message( "open62541LinkDir=${open62541Dir}/lib" )
	link_directories( ${open62541Dir}/lib )
	include_directories( ${open62541Dir}/include )
endif()
include_directories( $ENV{JDE_DIR}/Public/jde/app/shared/proto )

add_executable( ${CMAKE_PROJECT_NAME} )
file( GLOB sources *.cpp **/*.cpp )
file( GLOB headers *.h **/*.h )
#set( CMAKE_INCLUDE_CURRENT_DIR ON )
target_sources( ${CMAKE_PROJECT_NAME} PRIVATE ${sources} ${headers} )

find_package( GTest REQUIRED )
target_link_libraries( ${CMAKE_PROJECT_NAME}  GTest::gtest_main )
if( MSVC )
	target_link_libraries( ${CMAKE_PROJECT_NAME} fmt::fmt )
else()
	target_link_libraries( ${CMAKE_PROJECT_NAME} libopen62541.a )
	target_link_libraries( ${CMAKE_PROJECT_NAME} lzma )
	target_link_libraries( ${CMAKE_PROJECT_NAME} gtest_main)
	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		target_link_libraries( ${CMAKE_PROJECT_NAME} stdc++)
		target_link_libraries( ${CMAKE_PROJECT_NAME} stdc++fs)
	endif()
	target_link_libraries( ${CMAKE_PROJECT_NAME} libcrypto.a)
	target_link_libraries( ${CMAKE_PROJECT_NAME} debug libfmtd.a optimized libfmt.a)

	target_link_libraries( ${CMAKE_PROJECT_NAME} libutf8_range.a libutf8_validity.a )
	target_link_libraries( ${CMAKE_PROJECT_NAME} absl::cord absl::log_internal_check_op absl::log_internal_conditions absl::log_internal_message absl::raw_hash_set absl::status absl::statusor ) #absl::strings
	target_link_libraries( ${CMAKE_PROJECT_NAME} Jde Jde.App.Shared Jde.App.Client Jde.Crypto Jde.Iot Jde.Web.Client Jde.Web.Server )
endif()
target_link_libraries( ${CMAKE_PROJECT_NAME} protobuf::libprotobuf )
target_link_libraries( ${CMAKE_PROJECT_NAME} OpenSSL::SSL )

target_precompile_headers( ${CMAKE_PROJECT_NAME} PRIVATE
	<gtest/gtest.h>
	<boost/asio.hpp>
	<boost/beast.hpp>
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
	<jde/TypeDefs.h>
	<jde/log/Log.h>
	<jde/Exception.h>
	<jde/web/server/exports.h>
	<jde/iot/TypeDefs.h>
	<jde/iot/exports.h>
	<jde/app/shared/exports.h>
	"externals.h"
	"../../../Framework/source/db/Database.h"
)

if( MSVC )
	set( CMAKE_INSTALL_PREFIX $ENV{JDE_DIR}/installed )
	set( installDir ${CMAKE_INSTALL_PREFIX}/$<CONFIGURATION> )
	set( CUSTOM_RUNTIME_DIR ${installDir} )
	install( TARGETS ${CMAKE_PROJECT_NAME} RUNTIME DESTINATION ${CUSTOM_RUNTIME_DIR} )
	install( FILES $<TARGET_RUNTIME_DLLS:${CMAKE_PROJECT_NAME}> DESTINATION ${CUSTOM_RUNTIME_DIR} )
	install( FILES $<TARGET_PDB_FILE:${CMAKE_PROJECT_NAME}> CONFIGURATIONS Debug RelWithDebInfo DESTINATION ${installDir}/bin )
	set_target_properties( ${CMAKE_PROJECT_NAME} PROPERTIES
		VS_DEBUGGER_COMMAND_ARGUMENTS "-c"
		VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		VS_DEBUGGER_COMMAND ${installDir}/${CMAKE_PROJECT_NAME}.exe )
endif()