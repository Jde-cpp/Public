cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( ../../../build/functions.cmake )
set( targetName Jde.Access.Tests )
project( ${targetName} VERSION ${JDE_VERSION} LANGUAGES CXX )
include( ../../../build/dependencies.cmake )

#add_subdirectory( $ENV{JDE_DIR}/Framework/source ../../framework )
add_subdirectory( ../../db/src ../../db/lib )
if( WIN32 )
	add_subdirectory( ../../db/drivers/odbc ../../db/drivers/odbc )
else()
	add_subdirectory( ../../db/drivers/mysql ../../db/drivers/mysql )
endif()
add_subdirectory( ../../ql ../../ql )

add_executable( ${targetName} )
if( WIN32)
	set_target_properties( ${targetName} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>" )
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

set( CMAKE_CTEST_ARGUMENTS -c -settings=$ENV{JDE_DIR}/Public/libs/access/tests/config/Access.Tests.jsonnet )
add_test( NAME AccessTests COMMAND $<TARGET_FILE:Jde.Access.Tests> )

if( WIN32 )
	set_target_properties( ${targetName} PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "-tests -settings=${CMAKE_CURRENT_SOURCE_DIR}/config/Access.Tests.jsonnet" )
	add_custom_command( TARGET ${targetName} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${LIB_DIR}/fmt/bin/fmtd.dll" $<TARGET_FILE_DIR:${targetName}> )
	# add_custom_command( TARGET ${targetName} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy "${LIB_DIR}/fmt/bin/fmtd.dll" "${CMAKE_BINARY_DIR}/tests/${CMAKE_BUILD_TYPE_INIT}" COMMENT "copy fmt" )
	# add_custom_command( TARGET ${targetName} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/../framework/${CMAKE_BUILD_TYPE_INIT}/Jde.dll" "${CMAKE_BINARY_DIR}/tests/${CMAKE_BUILD_TYPE_INIT}" COMMENT "copy framework" DEPENDS ${CMAKE_BINARY_DIR}/../framework/${CMAKE_BUILD_TYPE_INIT}/Jde.dll )
	# add_custom_command( TARGET ${targetName} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/../framework/${CMAKE_BUILD_TYPE_INIT}/Jde.pdb" "${CMAKE_BINARY_DIR}/tests/${CMAKE_BUILD_TYPE_INIT}" )
	# add_custom_command( TARGET ${targetName} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/../db/lib/${CMAKE_BUILD_TYPE_INIT}/Jde.DB.dll" "${CMAKE_BINARY_DIR}/tests/${CMAKE_BUILD_TYPE_INIT}" COMMENT "copy db" )
	# add_custom_command( TARGET ${targetName} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/../db/lib/${CMAKE_BUILD_TYPE_INIT}/Jde.DB.pdb" "${CMAKE_BINARY_DIR}/tests/${CMAKE_BUILD_TYPE_INIT}" )
endif()

target_precompile_headers(${targetName}
  PRIVATE
		"pc.h"
)