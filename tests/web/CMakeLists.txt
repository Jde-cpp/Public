cmake_minimum_required(VERSION 3.16)
project( Tests.Web VERSION 1.0.0.0 )
set( CMAKE_MODULE_PATH $ENV{JDE_DIR}/Public/build )
include(common)
include(tests)
link_directories( ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} )
link_directories( ${fmt_DIR}/../.. )
link_directories( ${Protobuf_DIR}/../.. )
include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

#add_executable( ${CMAKE_PROJECT_NAME} main.cpp WebTests.cpp TestRequestAwait.cpp CertificateTests.cpp
#	proto/test.pb.cc )
add_executable( ${CMAKE_PROJECT_NAME} )
file(GLOB src_files *.cpp **/*.cpp )
file(GLOB hdr_files   "**/*.h")
file(GLOB proto_files "**/*.proto")
set(CMAKE_INCLUDE_CURRENT_DIR ON)
target_sources("${CMAKE_PROJECT_NAME}"
    PRIVATE
        "${src_files}"
        "${hdr_files}"
        "${proto_files}")

target_link_libraries( ${CMAKE_PROJECT_NAME} Jde )
target_link_libraries( ${CMAKE_PROJECT_NAME} Jde.Crypto )
target_link_libraries( ${CMAKE_PROJECT_NAME} Jde.Ssl )
target_link_libraries( ${CMAKE_PROJECT_NAME} Jde.Http )
target_link_libraries( ${CMAKE_PROJECT_NAME} Jde.App.Client )
target_link_libraries( ${CMAKE_PROJECT_NAME} Jde.Web )
target_link_libraries( ${CMAKE_PROJECT_NAME} Jde.XZ )
target_link_libraries( ${CMAKE_PROJECT_NAME} gtest_main )
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_link_libraries( ${CMAKE_PROJECT_NAME} stdc++ )
	target_link_libraries( ${CMAKE_PROJECT_NAME} stdc++fs )
endif()
target_link_libraries( ${CMAKE_PROJECT_NAME} absl::cord absl::log_internal_check_op absl::log_internal_conditions absl::log_internal_message absl::raw_hash_set absl::status absl::statusor ) #absl::strings
target_link_libraries( ${CMAKE_PROJECT_NAME} debug libfmtd.a optimized libfmt.a )
target_link_libraries( ${CMAKE_PROJECT_NAME} protobuf::libprotobuf libutf8_range.a libutf8_validity.a )
target_link_libraries( ${CMAKE_PROJECT_NAME} lzma )
target_link_libraries( ${CMAKE_PROJECT_NAME} libssl.a )
target_link_libraries( ${CMAKE_PROJECT_NAME} libcrypto.a )

target_precompile_headers( ${CMAKE_PROJECT_NAME} PRIVATE
	<gtest/gtest.h>
	<boost/asio.hpp>
	<boost/asio/ip/tcp.hpp>
	<boost/beast.hpp>
	<boost/beast/http.hpp>
	<boost/beast/ssl.hpp>
	<boost/beast/websocket.hpp>
	<jde/TypeDefs.h>
	<jde/Exception.h>
	<jde/Log.h>
	<jde/App.h>
	"../../../Framework/source/Settings.h"
	"../../../Framework/source/Stopwatch.h"
)

set( protoExeImportDir ${CMAKE_CURRENT_LIST_DIR}/.. )
file( REAL_PATH ../../src/http httpDir BASE_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}" )

set( importProtoConfigDir ${httpDir}/proto )
include_directories( ${httpDir}/.obj/${outDir} )

if( NOT EXISTS ${protoExeImportDir}/FromServer.proto )
	execute_process( COMMAND ${CMAKE_COMMAND} -E create_symlink ${importProtoConfigDir}/FromServer.proto ${protoExeImportDir}/FromServer.proto )
endif()
if( NOT EXISTS ${protoExeImportDir}/FromClient.proto )
	execute_process( COMMAND ${CMAKE_COMMAND} -E create_symlink ${importProtoConfigDir}/FromClient.proto ${protoExeImportDir}/FromClient.proto )
endif()
protobuf_generate(
	TARGET "${CMAKE_PROJECT_NAME}"
	IMPORT_DIRS ${protoExeImportDir} )
