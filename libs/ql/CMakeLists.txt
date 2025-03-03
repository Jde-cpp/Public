cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( ../../build/functions.cmake )
project( Jde.QL VERSION ${JDE_VERSION} LANGUAGES CXX )
include( ../../build/dependencies.cmake )
set( targetName Jde.QL )
add_library( ${targetName} )

file( GLOB sources *.cpp **/*.cpp **/**/*.cpp )
file( GLOB headers **/*.h $ENV{JDE_DIR}/Public/include/jde/ql )
target_sources(${targetName}
    PRIVATE
        ${sources}
        ${headers}
)

target_link_libraries( ${targetName} Jde.DB )

target_precompile_headers(${targetName}
  PUBLIC
  <jde/framework.h>
)