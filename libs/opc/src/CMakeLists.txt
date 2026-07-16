cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( ${CMAKE_CURRENT_LIST_DIR}/../../../build/functions.cmake )
message( Jde.Opc )
project( Jde.Opc VERSION ${JDE_VERSION} LANGUAGES CXX )
include( ${jdeRoot}/build/dependencies.cmake )
find_package( OpenSSL REQUIRED )
find_package( open62541 REQUIRED ) #open62541::open62541 (linked below) carries the include/lib dirs.
add_definitions( -DUA_ENABLE_ENCRYPTION )

set( targetName Jde.Opc )
add_library( ${targetName} )
compileOptions( ${targetName} )
file( GLOB_RECURSE sources *.cpp )
file( GLOB_RECURSE headers  *.h )

target_sources( ${targetName} PRIVATE ${sources} ${headers} ${protos} )

if( WIN32 )
else()
	target_link_libraries( ${targetName} stdc++exp )
endif()

if( WIN32 )
	target_link_libraries( ${targetName} ${CMAKE_INSTALL_PREFIX}/protobuf/lib/protobuf$<IF:$<CONFIG:Debug>,d,>.lib )
endif()

target_link_libraries( ${targetName} open62541::open62541 )
target_link_libraries( ${targetName} OpenSSL::SSL )
target_link_libraries( ${targetName} fmt::fmt )

target_precompile_headers(${targetName}
  PRIVATE
		"pc.h"
)
