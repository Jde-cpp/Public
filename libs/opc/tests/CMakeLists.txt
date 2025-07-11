cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( ../../../build/functions.cmake )
project( Jde.Opc.Tests VERSION ${JDE_VERSION} LANGUAGES CXX )
include( ../../../build/dependencies.cmake )


find_package( open62541 REQUIRED )
set( open62541Dir ${open62541_DIR}/../../.. )
cmake_path( ABSOLUTE_PATH open62541Dir NORMALIZE OUTPUT_VARIABLE open62541Dir )
link_directories( ${open62541Dir}/lib )
add_definitions( -DUA_ENABLE_ENCRYPTION )
if( WIN32 )
	add_definitions( -DZLIB_USE_STATIC_LIBS -D )
	set( ZLIB_ROOT ${LIB_DIR}/zlib )
endif()
find_package( ZLIB REQUIRED )

set( targetName Jde.Opc.Tests )
add_executable( ${targetName} )
compileOptions( ${targetName} )
if( WIN32)
	set( CMAKE_SHARED_LINKER_FLAGS_DEBUG "/debug /INCREMENTAL:NO" )
	set_target_properties( ${targetName} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>" )
endif()

file( GLOB sources *.cpp **/*.cpp )
if( WIN32 )
	list( APPEND sources OpcTests.rc )
endif()
file( GLOB headers *.h **/*.h )
target_sources( ${targetName} PRIVATE ${sources} ${headers} )

add_subdirectory( ../../crypto/src ../../crypto/lib )
add_subdirectory( ../../ql ../../ql )
add_subdirectory( ../../access/src ../../access/lib )
add_subdirectory( ../../app/shared ../../app/shared )
add_subdirectory( ../../app/client ../../app/client )
add_subdirectory( ../../web/client ../../web/client )
add_subdirectory( ../../web/server ../../web/server )
add_subdirectory( ../../db/src ../../db/lib )

find_package( GTest REQUIRED )
include_directories( ${open62541Dir}include )
target_link_libraries( ${targetName} GTest::gtest )
target_link_libraries( ${targetName} Boost::json )
target_link_libraries( ${targetName} Jde Jde.Crypto Jde.QL Jde.Access Jde.App.Shared Jde.App.Client Jde.Web.Client Jde.Web.Server Jde.Opc )
target_link_libraries( ${targetName} protobuf::libprotobuf )
if( WIN32 )
	target_link_libraries( ${targetName} ZLIB::ZLIB )
	target_link_libraries( ${targetName} Iphlpapi.lib )
else()
	target_link_libraries( ${targetName} OpenSSL::SSL )
	target_link_libraries( ${targetName} stdc++exp )#for stacktrace
endif()

if( WIN32 )
	set_target_properties( ${targetName} PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "-tests -settings=${CMAKE_CURRENT_SOURCE_DIR}/config/Opc.Tests.jsonnet" )
	copyCommonDlls()
endif()
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