cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( ../../../build/functions.cmake )
include( ../../../build/linkSo.cmake )

if( NOT WIN32 )
	if( NOT CMAKE_BUILD_TYPE STREQUAL "Debug" )
		set( JSONNET_DIR ${LIB_DIR}/jsonnet CACHE PATH "Jsonnet directory" )
		link_directories( ${JSONNET_DIR}/lib )
	endif()
endif()

set( targetName Jde.Access.Tests )
project( ${targetName} VERSION ${JDE_VERSION} LANGUAGES CXX )
include( ../../../build/dependencies.cmake )

add_subdirectory( ../../fwk/src ../../fwk/lib )
#add_subdirectory( ../../db/src ../../db/lib ) should be with the driver
if( WIN32 )
	add_subdirectory( ../../db/drivers/odbc ../../db/drivers/odbc )
else()
	add_subdirectory( ../../db/drivers/mysql ../../db/drivers/mysql )
endif()
add_subdirectory( ../../ql ../../ql )
#add_subdirectory( ../../crypto/src ../../crypto/lib )

add_executable( ${targetName} )
if( WIN32)
#	set( CMAKE_EXE_LINKER_FLAGS_DEBUG "/debug /INCREMENTAL:NO" )
#	set_target_properties( ${targetName} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>" )
	set_target_properties( ${targetName} PROPERTIES LINK_FLAGS ${CMAKE_EXE_LINKER_FLAGS} )
endif()

compileOptions( ${targetName} )
file(GLOB sources *.cpp )
file(GLOB headers "**/*.h")
target_sources(${targetName} PRIVATE ${sources} ${headers} )

find_package( GTest REQUIRED )
target_link_libraries( ${targetName} Boost::json )
target_link_libraries( ${targetName} GTest::gtest )
target_link_libraries( ${targetName} Jde.Access Jde.DB Jde.QL Jde )
target_compile_definitions( ${targetName} PRIVATE TESTS )

if( WIN32 )
	set_target_properties( ${targetName} PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "-tests -settings=${CMAKE_CURRENT_SOURCE_DIR}/config/Access.Tests.jsonnet" )
	copyCommonDlls()
else()
	if( NOT CMAKE_BUILD_TYPE STREQUAL "Debug" )
		target_link_libraries( ${targetName} jsonnet++ jsonnet )
		linkRepo( ${CMAKE_BINARY_DIR}/.. ${CMAKE_BINARY_DIR}/tests )
	endif()
endif()

add_test( NAME AccessTests COMMAND $<TARGET_FILE:Jde.Access.Tests> -tests -settings=${CMAKE_CURRENT_SOURCE_DIR}/config/Access.Tests.jsonnet )

target_precompile_headers(${targetName}
  PRIVATE
		"pc.h"
)