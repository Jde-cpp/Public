cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( ../../../build/functions.cmake )
project( Jde.Web.Server VERSION ${JDE_VERSION} LANGUAGES CXX )
include( ../../../build/dependencies.cmake )

find_package( OpenSSL REQUIRED )

#link_directories( ${Protobuf_DIR}/../.. )
add_definitions( -DJde_Web_Server_EXPORTS )

set( targetName Jde.Web.Server )
add_library( ${targetName} )

file(GLOB sources *.cpp )
file(GLOB headers   *.h ../../../jde/web/server/*.h )
target_sources( ${targetName} PRIVATE ${sources} ${headers} )

target_link_libraries( ${targetName} protobuf::libprotobuf )
target_link_libraries( ${targetName} Jde.QL Jde.Crypto Jde.Web.Client )

target_precompile_headers( ${targetName}
  PRIVATE
	"pc.h"
)