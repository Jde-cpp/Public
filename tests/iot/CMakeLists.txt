cmake_minimum_required(VERSION 3.16)
project( Tests.Iot VERSION 1.0.0.0 )
set( CMAKE_MODULE_PATH $ENV{JDE_DIR}/Public/build )
include(common)
include(tests)
link_directories( ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} )
link_directories( ${fmt_DIR}/../.. )
link_directories( ${Protobuf_DIR}/../.. )

find_package(open62541 REQUIRED)
set( open62541Dir ${open62541_DIR}/../../.. )
message( "open62541LinkDir=${open62541Dir}/lib" )
link_directories( ${open62541Dir}/lib )
include_directories( ${open62541Dir}/include )
include_directories( $ENV{JDE_DIR}/Public/jde/app/shared/proto )

add_executable( ${CMAKE_PROJECT_NAME} main.cpp helpers.cpp OpcServerTests.cpp
	uatypes/UAClientTests.cpp )

target_link_libraries( ${CMAKE_PROJECT_NAME} libopen62541.a )
target_link_libraries( ${CMAKE_PROJECT_NAME} lzma )
target_link_libraries( ${CMAKE_PROJECT_NAME} gtest_main)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_link_libraries( ${CMAKE_PROJECT_NAME} stdc++)
	target_link_libraries( ${CMAKE_PROJECT_NAME} stdc++fs)
endif()
target_link_libraries( ${CMAKE_PROJECT_NAME} libcrypto.a)
target_link_libraries( ${CMAKE_PROJECT_NAME} debug libfmtd.a optimized libfmt.a)

target_link_libraries( ${CMAKE_PROJECT_NAME} protobuf::libprotobuf libutf8_range.a libutf8_validity.a )
target_link_libraries( ${CMAKE_PROJECT_NAME} absl::cord absl::log_internal_check_op absl::log_internal_conditions absl::log_internal_message absl::raw_hash_set absl::status absl::statusor ) #absl::strings
target_link_libraries( ${CMAKE_PROJECT_NAME} Jde Jde.App.Shared Jde.App.Client Jde.Crypto Jde.Iot Jde.Web.Client Jde.Web.Server )

target_precompile_headers( ${CMAKE_PROJECT_NAME} PRIVATE
	<gtest/gtest.h>
	<boost/asio.hpp>
	<boost/beast.hpp>
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
	<jde/iot/types/proto/Iot.Common.pb.h>
	<jde/iot/types/proto/Iot.FromClient.pb.h>
	<jde/iot/types/proto/Iot.FromServer.pb.h>
	<jde/TypeDefs.h>
	<jde/log/Log.h>
	<jde/Exception.h>
	<jde/web/server/exports.h>
	"../../../Framework/source/db/Database.h"
)