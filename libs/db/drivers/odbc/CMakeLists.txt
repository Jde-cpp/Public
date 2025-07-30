cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( ../../../../build/functions.cmake )
set( targetName Jde.DB.Odbc )
project( ${targetName} VERSION ${JDE_VERSION} LANGUAGES CXX )
include( ../../../../build/dependencies.cmake )
add_library( ${targetName} SHARED )
compileOptions( ${targetName} )

file(GLOB sources *.cpp MsSql/*.cpp )
file(GLOB headers   *.h )

target_sources( ${targetName} PRIVATE ${sources} ${headers} )

if( WIN32 )
	set( CMAKE_SHARED_LINKER_FLAGS_DEBUG "/debug /INCREMENTAL:NO" )
	target_link_libraries( ${targetName} odbc32.dll )
	target_link_libraries( ${targetName} Jde Jde.DB )
endif()

target_precompile_headers(${targetName}
	PRIVATE
	"pc.h"
)
if( WIN32 )
	set_target_properties( ${targetName} PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "-settings=${CMAKE_CURRENT_SOURCE_DIR}/config/Crypto.Tests.jsonnet" )
endif()