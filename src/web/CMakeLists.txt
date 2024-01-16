cmake_minimum_required(VERSION 3.16)
project(Web)
set( CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../../../Framework/cmake )
include(SetUpConfigurations)
#######################################################
#if(MSVC)
#	add_compile_options(PRIVATE /W4 /WX)
#else()
#	add_compile_options( -Wall -Wno-unknown-pragmas )
#endif()
#######################################################
#add_link_options( "LINKER:-z,origin-shared" )
#add_link_options( "LINKER:-rpath=$ORIGIN" )
set(CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS} "-Wl,-rpath=$ORIGIN")
add_definitions( -DJde_WEB_EXPORTS )
find_package(OpenSSL)
if(MSVC)
	add_definitions( -DWIN32_LEAN_AND_MEAN )
else()
	target_link_libraries(Jde.Web Threads::Threads)
endif()
add_library( Jde.Web SHARED ProtoServer.cpp RestServer.cpp WebSocket.cpp
	proto/FromServer.pb.cc )
#######################################################

#######################################################
target_precompile_headers(Jde.Web
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
