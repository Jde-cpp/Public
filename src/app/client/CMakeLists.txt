cmake_minimum_required(VERSION 3.16)
project(Jde.App.Client)
set( CMAKE_MODULE_PATH $ENV{JDE_DIR}/Public/build )
include(common)
link_directories( ${Protobuf_DIR}/../.. )
#######################################################
set(CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS} "-Wl,-rpath=$ORIGIN")
add_definitions( -DJde_APP_CLIENT_EXPORTS )
find_package(OpenSSL)
add_library( ${CMAKE_PROJECT_NAME} SHARED )
set( CMAKE_INCLUDE_CURRENT_DIR ON )
file(GLOB src_files *.cpp await/*.cpp )
file(GLOB hdr_files  **/*.h )
target_sources( "${CMAKE_PROJECT_NAME}"
    PRIVATE
        "${src_files}"
        "${hdr_files}")

target_link_libraries( ${CMAKE_PROJECT_NAME} debug libfmtd.a optimized libfmt.a )
target_link_libraries( ${CMAKE_PROJECT_NAME} protobuf::libprotobuf libutf8_range.a )
if(MSVC)
	add_definitions( -DWIN32_LEAN_AND_MEAN )
else()
	target_link_libraries(${CMAKE_PROJECT_NAME} Threads::Threads)
endif()
#######################################################
target_precompile_headers(${CMAKE_PROJECT_NAME}
  PRIVATE
	<boost/beast.hpp>
	<boost/asio.hpp>
	<boost/beast/ssl/ssl_stream.hpp>

#	<boost/exception/diagnostic_information.hpp>
<jde/TypeDefs.h>
<jde/log/Log.h>
#	<jde/web/usings.h>
#	<jde/coroutine/Task.h>
<jde/app/shared/proto/App.FromClient.pb.h>
<jde/app/shared/proto/App.FromServer.pb.h>
"../../../../Framework/source/coroutine/Awaitable.h"
#	"../../../Framework/source/db/GraphQL.h"
#	"../../../Framework/source/io/AsioContextThread.h"
#	"../../../Framework/source/io/Socket.h"
#	"../../../Framework/source/io/ProtoClient.h"
#	"../../../Framework/source/io/ProtoUtilities.h"
#	"../../../Framework/source/threading/InterruptibleThread.h"
#	"../../../Ssl/source/Ssl.h"
)

