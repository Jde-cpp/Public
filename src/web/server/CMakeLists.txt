cmake_minimum_required(VERSION 3.20)
set( CMAKE_MODULE_PATH $ENV{JDE_DIR}/Public/build )
include(common)
findBoost()
if( ${VCPKG} )
	SET( CMAKE_TOOLCHAIN_FILE $ENV{REPO_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake  CACHE STRING "Vcpkg toolchain file" )
endif()
project(Jde.Web.Server)
project( Jde.Web.Client )
include( compiler )

find_package(OpenSSL)
if( MSVC )
include_directories( ${OPENSSL_INCLUDE_DIR} )
	link_directories( $ENV{JDE_DIR}/Framework/source/.build/.bin/${outDir} )
	link_directories( $ENV{JDE_DIR}/Public/src/crypto/.build/.bin/${outDir} )
else()
	link_directories( ${Protobuf_DIR}/../.. )
	add_definitions( -DJde_WEB_SERVER_EXPORTS )
endif()
#######################################################
add_library( ${CMAKE_PROJECT_NAME} SHARED )
file(GLOB src_files *.cpp )
file(GLOB hdr_files   "**/*.h")
target_sources( "${CMAKE_PROJECT_NAME}"
    PRIVATE
        "${src_files}"
        "${hdr_files}")

if(MSVC)
<<<<<<< HEAD
	find_package( fmt )
	find_package( ZLIB )

	target_link_libraries( ${CMAKE_PROJECT_NAME} Jde )
	target_link_libraries( ${CMAKE_PROJECT_NAME} fmt::fmt )
	target_link_libraries( ${CMAKE_PROJECT_NAME} ZLIB::ZLIB )
=======
>>>>>>> 2a3c326db5998c266a3eb61f540584f896dd5c81
else()
	target_link_libraries( ${CMAKE_PROJECT_NAME} Threads::Threads )
	target_link_libraries( ${CMAKE_PROJECT_NAME} debug libfmtd.a optimized libfmt.a )
	target_link_libraries( ${CMAKE_PROJECT_NAME} protobuf::libprotobuf libutf8_range.a )# libutf8_validity.a )
endif()
target_link_libraries( ${CMAKE_PROJECT_NAME} OpenSSL::SSL )
#######################################################
target_precompile_headers(${CMAKE_PROJECT_NAME}
  PRIVATE
	<boost/beast.hpp>
	<boost/asio.hpp>
	<boost/exception/diagnostic_information.hpp>
	<jde/log/Log.h>
	<jde/web/server/usings.h>
	<jde/coroutine/Task.h>
	"../../../../Framework/source/coroutine/Awaitable.h"
	"../../../../Framework/source/db/GraphQL.h"
	"../../../../Framework/source/io/AsioContextThread.h"
	"../../../../Framework/source/io/ProtoUtilities.h"
	"../../../../Framework/source/threading/InterruptibleThread.h"
)