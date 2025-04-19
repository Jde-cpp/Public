cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( ../../../build/functions.cmake )
project( Jde.Access.Tests VERSION ${JDE_VERSION} LANGUAGES CXX )
include( ../../../build/dependencies.cmake )

add_subdirectory( $ENV{JDE_DIR}/Framework/source ../../framework )
add_subdirectory( ../../db/src ../../db/lib )
add_subdirectory( ../../db/drivers/mysql ../../db/drivers/mysql )
add_subdirectory( ../../ql ../../ql )


set( targetName Jde.Access.Tests )
add_executable( ${targetName} )
file(GLOB sources *.cpp )
file(GLOB headers "**/*.h")
target_sources(${targetName} PRIVATE ${sources} ${headers} )

if( WIN32 )
	set_target_properties( ${CMAKE_PROJECT_NAME} PROPERTIES
		VS_DEBUGGER_COMMAND_ARGUMENTS "-c"
		VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		VS_DEBUGGER_COMMAND ${installDir}/${CMAKE_PROJECT_NAME}.exe )
endif()
find_package( GTest REQUIRED )
target_link_libraries( ${targetName} Boost::json )
target_link_libraries( ${targetName} GTest::gtest )
target_link_libraries( ${targetName} Jde.Access Jde.DB Jde.QL Jde )

set( CMAKE_CTEST_ARGUMENTS -c -settings=$ENV{JDE_DIR}/Public/libs/access/tests/config/Access.Tests.jsonnet )
add_test( NAME AccessTests COMMAND $<TARGET_FILE:Jde.Access.Tests> )

target_precompile_headers(${targetName}
  PUBLIC
		"pc.h"
)
