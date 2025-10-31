cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( ../../../build/functions.cmake )
set( targetName Jde.Access.Tests )
project( ${targetName} VERSION ${JDE_VERSION} LANGUAGES CXX )
include( ../../../build/dependencies.cmake )

add_subdirectory( ../../db/src ../../db/lib )
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

#set( CMAKE_CTEST_ARGUMENTS -tests -settings=$ENV{JDE_DIR}/Public/libs/access/tests/config/Access.Tests.jsonnet )
add_test( NAME AccessTests COMMAND $<TARGET_FILE:Jde.Access.Tests> -tests -settings=${CMAKE_CURRENT_SOURCE_DIR}/config/Access.Tests.jsonnet )

if( WIN32 )
	set_target_properties( ${targetName} PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "-tests -settings=${CMAKE_CURRENT_SOURCE_DIR}/config/Access.Tests.jsonnet" )
	copyCommonDlls()
endif()

target_precompile_headers(${targetName}
  PRIVATE
		"pc.h"
)