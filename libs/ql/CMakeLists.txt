cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( ../../build/functions.cmake )
set( targetName Jde.QL )
project( ${targetName} VERSION ${JDE_VERSION} LANGUAGES CXX )
include( ../../build/dependencies.cmake )
add_library( ${targetName} )
compileOptions( ${targetName} )
file( GLOB sources *.cpp **/*.cpp **/**/*.cpp )
file( GLOB headers **/*.h $ENV{JDE_DIR}/include/jde/ql )
target_sources(${targetName}
	PRIVATE
		${sources}
		${headers}
)

target_link_libraries( ${targetName} Jde Jde.DB )
target_link_libraries( ${targetName} Boost::json )

target_precompile_headers(${targetName}
  PUBLIC
  <jde/fwk.h>
)