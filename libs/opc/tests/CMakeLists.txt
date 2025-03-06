cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( ../../../build/functions.cmake )
project( Jde.Opc.Tests VERSION ${JDE_VERSION} LANGUAGES CXX )
include( ../../../build/dependencies.cmake )

link_directories( ${JSONNET_DIR}/lib )

set( targetName Jde.Opc.Tests )
add_executable( ${targetName} )
file( GLOB sources *.cpp **/*.cpp )
file( GLOB headers *.h **/*.h )
target_sources( ${targetName} PRIVATE ${sources} ${headers} )

if( WIN32 )
	set_target_properties( ${CMAKE_PROJECT_NAME} PROPERTIES
		VS_DEBUGGER_COMMAND_ARGUMENTS "-c"
		VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		VS_DEBUGGER_COMMAND ${installDir}/${CMAKE_PROJECT_NAME}.exe )
endif()
find_package( GTest REQUIRED )

include_directories( ${open62541Dir}include )
target_link_libraries( ${targetName} OpenSSL::SSL )
target_link_libraries( ${targetName} GTest::gtest )
target_link_libraries( ${targetName} Boost::json )
target_link_libraries( ${targetName} jsonnet )
target_link_libraries( ${targetName} Jde Jde.Crypto Jde.QL Jde.Access Jde.App.Shared Jde.App.Client Jde.Web.Client Jde.Opc )
target_link_libraries( ${targetName} protobuf::libprotobuf )
target_link_libraries( ${targetName} stdc++exp )#for stacktrace

target_precompile_headers( ${targetName} PRIVATE
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
	<jde/framework.h>
	<jde/web/server/exports.h>
	<jde/opc/usings.h>
	<jde/opc/exports.h>
	<jde/app/shared/exports.h>
	externals.h
)