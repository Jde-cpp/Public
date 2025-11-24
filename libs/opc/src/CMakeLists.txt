cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( ../../../build/functions.cmake )
message( Jde.Opc )
project( Jde.Opc VERSION ${JDE_VERSION} LANGUAGES CXX )
include( ../../../build/dependencies.cmake )
if( WIN32 )
else()
	find_package( OpenSSL )
endif()
find_package( open62541 REQUIRED )
set( open62541Dir ${open62541_DIR}/../../.. )
cmake_path( ABSOLUTE_PATH open62541Dir NORMALIZE OUTPUT_VARIABLE open62541Dir )
link_directories( ${open62541Dir}/lib )
add_definitions( -DUA_ENABLE_ENCRYPTION )

set( targetName Jde.Opc )
add_library( ${targetName} )
compileOptions( ${targetName} )
file( GLOB_RECURSE sources *.cpp )
file( GLOB_RECURSE headers  *.h )

target_include_directories( ${targetName} PUBLIC ${open62541Dir}include )

target_sources( ${targetName} PRIVATE ${sources} ${headers} ${protos} )

if( WIN32 )
#	target_link_libraries( ${targetName} boost::json )
else()
	target_link_libraries( ${targetName} stdc++exp )
endif()
target_link_libraries( ${targetName} protobuf::libprotobuf )
target_link_libraries( ${targetName} open62541::open62541 )

target_precompile_headers(${targetName}
  PRIVATE
		"pc.h"
)
