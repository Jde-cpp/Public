cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( ../../../build/functions.cmake )
project( Jde.App.DB VERSION ${JDE_VERSION} LANGUAGES CXX )
include( ../../../build/dependencies.cmake )

set( targetName Jde.DB )
add_library( ${targetName} SHARED )

set_property( TARGET ${targetName} PROPERTY POSITION_INDEPENDENT_CODE ON )

file( GLOB sources awaits/*.cpp generators/*.cpp *.cpp meta/*.cpp meta/ddl/*.cpp )
file( GLOB headers *.h ../../jde/db/*.h ../../jde/db/meta/*.h ../../jde/db/syntax/*.h )
target_sources(${targetName}
    PRIVATE
        ${sources}
        ${headers}
)

target_precompile_headers(${targetName}
  PUBLIC
  <jde/framework.h>
)
