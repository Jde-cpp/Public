cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( $ENV{JDE_DIR}/Public/build/functions.cmake )
project( Jde.App.Server.Tests ${JDE_VERSION} LANGUAGES CXX )
include( $ENV{JDE_DIR}/Public/build/dependencies.cmake )

set( targetName Jde.App.Server.Tests )
add_executable( ${targetName} )
file( GLOB sources *.cpp )
file( GLOB_RECURSE headers  *.h )
target_sources( ${targetName} PRIVATE ${sources} ${headers} )

find_package( GTest REQUIRED )

target_precompile_headers( ${targetName} PRIVATE
	<jde/fwk.h>
	"externals.h"
)