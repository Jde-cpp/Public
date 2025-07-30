cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( ../../../build/functions.cmake )
message( "Jde.DB" )
set( targetName Jde.DB )
project( ${targetName} VERSION ${JDE_VERSION} LANGUAGES CXX )
include( ../../../build/dependencies.cmake )

if( WIN32 )
	set( CMAKE_SHARED_LINKER_FLAGS_DEBUG "/debug /INCREMENTAL:NO" )
	link_directories( ${CMAKE_BINARY_DIR}/../framework/${CMAKE_BUILD_TYPE_INIT} )
endif()
add_library( ${targetName} SHARED )
compileOptions( ${targetName} )

file( GLOB sources awaits/*.cpp generators/*.cpp *.cpp meta/*.cpp meta/ddl/*.cpp )
file( GLOB headers *.h ../../jde/db/*.h ../../jde/db/meta/*.h ../../jde/db/syntax/*.h )
target_sources(${targetName}
    PRIVATE
        ${sources}
        ${headers}
)
if( WIN32 )
	target_link_libraries( ${targetName} Boost::json )
	target_link_libraries( ${targetName} fmt::fmt )
	target_link_libraries( ${targetName} Jde )
endif()

target_precompile_headers(${targetName}
  PUBLIC
  <jde/framework.h>
)