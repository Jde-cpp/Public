cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( ../../../build/functions.cmake )
project( Jde.App.Client VERSION ${JDE_VERSION} LANGUAGES CXX )
include( ../../../build/dependencies.cmake )

find_package( OpenSSL REQUIRED )
include_directories( $ENV{JDE_DIR}/Public/include/jde/web/client/proto )
#link_directories( ${Protobuf_DIR}/../.. )

set( targetName Jde.App.Client )
add_library( ${targetName} )
file(GLOB sources *.cpp awaits/*.cpp )
file(GLOB headers  **/*.h )
target_sources( ${targetName}
    PRIVATE
        "${sources}"
        "${headers}")

add_definitions( -DJde_App_Client_EXPORTS )
#target_link_libraries( ${targetName} OpenSSL::SSL )
target_link_libraries( ${targetName} Jde.App.Shared )
target_precompile_headers(${targetName}
  PRIVATE
	pc.h
)
