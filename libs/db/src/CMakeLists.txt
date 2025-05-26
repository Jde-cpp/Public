cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( ../../../build/functions.cmake )
set( targetName Jde.DB )
project( ${targetName} VERSION ${JDE_VERSION} LANGUAGES CXX )
include( ../../../build/dependencies.cmake )

if( WIN32 )
#	message( link_dir=${CMAKE_BINARY_DIR}/../framework/${CMAKE_BUILD_TYPE_INIT})
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

if( WIN32 )
	#set_target_properties( ${targetName} PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "-settings=${CMAKE_CURRENT_SOURCE_DIR}/config/Crypto.Tests.jsonnet" )
	#add_custom_command( TARGET ${targetName} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy "${LIB_DIR}/fmt/bin/fmtd.dll" "${CMAKE_BINARY_DIR}/tests/${CMAKE_BUILD_TYPE_INIT}" COMMENT "copy fmt" )
#	message( 'copy: "${CMAKE_BINARY_DIR}/../framework/${CMAKE_BUILD_TYPE_INIT}/Jde.lib to ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE_INIT}"' )
#	add_custom_command( TARGET ${targetName} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/../framework/${CMAKE_BUILD_TYPE_INIT}/Jde.lib" "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE_INIT}" COMMENT "db copy framework" )
endif()