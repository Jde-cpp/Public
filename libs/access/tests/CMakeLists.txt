cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( ${CMAKE_CURRENT_LIST_DIR}/../../../build/functions.cmake )

if( NOT WIN32 )
	if( NOT CMAKE_BUILD_TYPE STREQUAL "Debug" )
		set( jsonnet_DIR ${MULTI_INSTALL_PREFIX}/jsonnet CACHE PATH "Jsonnet directory" )
		link_directories( ${jsonnet_DIR}/lib )
	endif()
endif()

set( targetName Jde.Access.Tests )
project( ${targetName} VERSION ${JDE_VERSION} LANGUAGES CXX )
include( ${jdeRoot}/build/dependencies.cmake )

add_executable( ${targetName} )
if( WIN32)
	add_dependencies( ${targetName} Jde.DB.Odbc )
else()
	add_dependencies( ${targetName} Jde.DB.MySql )
endif()

compileOptions( ${targetName} )
file(GLOB sources *.cpp )
file(GLOB headers "*/*.h")
target_sources(${targetName} PRIVATE ${sources} ${headers} )

find_package( GTest REQUIRED )
if( Boost_USE_STATIC_LIBS )
	target_link_libraries( ${targetName} Boost::json )
endif()
target_link_libraries( ${targetName} GTest::gtest )
target_link_libraries( ${targetName} Jde.Access Jde.DB Jde.QL Jde )
target_compile_definitions( ${targetName} PRIVATE TESTS )

if( WIN32 )
	set_target_properties( ${targetName} PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "-tests -settings=${CMAKE_CURRENT_SOURCE_DIR}/config/Access.Tests.jsonnet" )
	copyCommonDlls()
else()
	if( NOT CMAKE_BUILD_TYPE STREQUAL "Debug" )
		target_link_libraries( ${targetName} libjsonnet++.so libjsonnet.so ) #jsonnet
	endif()
endif()

add_test( NAME AccessTests COMMAND $<TARGET_FILE:Jde.Access.Tests> -tests -settings=${CMAKE_CURRENT_SOURCE_DIR}/config/Access.Tests.jsonnet )

target_precompile_headers(${targetName}
  PRIVATE
		"pc.h"
)