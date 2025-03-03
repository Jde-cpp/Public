cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( ../../../build/functions.cmake )
project( Jde.AccessLib VERSION ${JDE_VERSION} LANGUAGES CXX )
include( ../../../build/dependencies.cmake )

set( targetName Jde.Access )
add_library( ${targetName} )

#add_definitions( -DJde_UM_EXPORTS )

file( GLOB sources *.cpp awaits/*.cpp events/*.cpp config/*.cpp hooks/*.cpp types/*.cpp )
file( GLOB headers *.h $ENV{JDE_DIR}/Public/include/jde/access/*.h $ENV{JDE_DIR}/Public/include/jde/access/types/*.h )
target_sources(${targetName} PRIVATE ${sources} ${headers} )
target_link_libraries( ${targetName} Jde.DB Jde.QL )

target_precompile_headers(${targetName}
  PUBLIC
  <jde/framework.h>
	<jde/framework/io/json.h>
	<jde/framework/coroutine/Await.h>
)
