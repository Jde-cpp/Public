cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( ${CMAKE_CURRENT_LIST_DIR}/../../../build/functions.cmake )
message( "Jde.DB" )
set( targetName Jde.DB )
project( ${targetName} VERSION ${JDE_VERSION} LANGUAGES CXX )
include( ${jdeRoot}/build/dependencies.cmake )

if( WIN32 )
	link_directories( ${CMAKE_BINARY_DIR}/../framework/${CMAKE_BUILD_TYPE_INIT} )
endif()
add_library( ${targetName} SHARED )
compileOptions( ${targetName} )

file( GLOB sources awaits/*.cpp generators/*.cpp *.cpp meta/*.cpp meta/ddl/*.cpp )
file( GLOB headers *.h ${jdeRoot}/include/jde/db/*.h ${jdeRoot}/include/jde/db/meta/*.h ${jdeRoot}/include/jde/db/syntax/*.h )
target_sources(${targetName}
    PRIVATE
        ${sources}
        ${headers}
)

if( Boost_USE_STATIC_LIBS )
	target_link_libraries( ${targetName} PRIVATE Boost::json )
endif()
target_link_libraries( ${targetName} PRIVATE fmt::fmt )
if( WIN32 )
	target_link_libraries( ${targetName} PRIVATE Jde )
endif()

target_precompile_headers(${targetName}
  PUBLIC
  <jde/fwk.h>
)