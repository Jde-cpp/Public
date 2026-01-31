cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( ../../../build/functions.cmake )
project( Jde.Web.Server VERSION ${JDE_VERSION} LANGUAGES CXX )
include( ../../../build/dependencies.cmake )

#find_package( OpenSSL REQUIRED )

#link_directories( ${Protobuf_DIR}/../.. )
add_definitions( -DJde_Web_Server_EXPORTS )

set( targetName Jde.Web.Server )
add_library( ${targetName} )
compileOptions( ${targetName} )
if( WIN32 )
	target_compile_options( ${targetName} PRIVATE /bigobj )
	target_link_directories( ${targetName} PUBLIC ${MULTI_INSTALL_PREFIX}/protobuf/lib )
endif()

file( GLOB sources *.cpp auth/*.cpp )
file( GLOB headers   *.h ../../../jde/web/server/*.h )
target_sources( ${targetName} PRIVATE ${sources} ${headers} )

if( WIN32 )
	target_link_libraries( ${targetName} libprotobuf$<IF:$<CONFIG:Debug>,d,>.lib )
	target_link_libraries( ${targetName} libutf8_range )
else()
	target_link_libraries( ${targetName} protobuf::libprotobuf )
endif()
target_link_libraries( ${targetName} Jde.QL Jde.Web.Client Jde.Access )

target_precompile_headers( ${targetName}
  PRIVATE
	"pc.h"
)