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

add_library( Jde.Web SHARED ProtoServer.cpp RestServer.cpp )
#######################################################
target_link_libraries(Jde.Web Threads::Threads)
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
	"../../..//Ssl/source/Ssl.h"
)
