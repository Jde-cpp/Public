cmake_minimum_required(VERSION 3.16)
project(Jde.Web)
set( CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../../build )
include(common)
link_directories( ${Protobuf_DIR}/../.. )
#######################################################
set(CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS} "-Wl,-rpath=$ORIGIN")
add_definitions( -DJde_WEB_EXPORTS )
find_package(OpenSSL)
add_library( ${CMAKE_PROJECT_NAME} SHARED ProtoServer.cpp RestServer.cpp WebSocket.cpp 
	proto/FromServer.pb.cc )

target_link_libraries( ${CMAKE_PROJECT_NAME} debug libfmtd.a optimized libfmt.a )
target_link_libraries( ${CMAKE_PROJECT_NAME} protobuf::libprotobuf libutf8_range.a )# libutf8_validity.a )
if(MSVC)
	add_definitions( -DWIN32_LEAN_AND_MEAN )
else()
	target_link_libraries(${CMAKE_PROJECT_NAME} Threads::Threads)
endif()
#######################################################
target_precompile_headers(${CMAKE_PROJECT_NAME}
  PRIVATE
	"TypeDefs.h"
	<jde/coroutine/Task.h>
	"../../../Framework/source/coroutine/Awaitable.h"
	"../../../Framework/source/db/GraphQL.h"
	"../../../Framework/source/io/AsioContextThread.h"
	"../../../Framework/source/io/Socket.h"
	"../../../Framework/source/io/ProtoClient.h"
	"../../../Framework/source/io/ProtoUtilities.h"
	"../../../Framework/source/threading/InterruptibleThread.h"
	"../../../Ssl/source/Ssl.h"
)
message( CMAKE_LIBRARY_OUTPUT_DIRECTORY=${CMAKE_LIBRARY_OUTPUT_DIRECTORY} )
