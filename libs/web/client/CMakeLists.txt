cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( ../../../build/functions.cmake )
message( Jde.Web.Client)
project( Jde.Web.Client VERSION ${JDE_VERSION} LANGUAGES CXX )
include( ../../../build/dependencies.cmake )

if( WIN32 )
	set( ZLIB_ROOT ${CMAKE_INSTALL_PREFIX}/zlib )
	find_package( OpenSSL REQUIRED )
endif()
find_package( ZLIB REQUIRED )
include_directories( ${ZLIB_INCLUDE_DIRS} )

set( targetName Jde.Web.Client )
add_library( ${targetName} )
compileOptions( ${targetName} )
set( CMAKE_INCLUDE_CURRENT_DIR ON )
file( GLOB sources *.cpp http/*.cpp socket/*.cpp )
set( includeDir $ENV{JDE_DIR}/include/jde/web/client )
file( GLOB headers *.h ${includeDir}/*.h ${includeDir}/http/*.h ${includeDir}/socket/*.h  )
file( GLOB protos proto/*.proto )
target_sources( ${targetName} PRIVATE ${sources} ${headers} ${protos} )

target_link_libraries( ${targetName} PUBLIC ZLIB::ZLIB Boost::json OpenSSL::SSL )

target_precompile_headers(${targetName}
  PUBLIC
	"pc.h"
)

protobuf_generate(
	TARGET ${targetName}
	IMPORT_DIRS proto
	PROTOC_OUT_DIR ${PROJECT_SOURCE_DIR}/proto
	EXPORT_MACRO Jde_Web_Client_EXPORT
)
suppressProtoWarnings( "${protos}" ${PROJECT_SOURCE_DIR}/proto )
set( protoIncludeDir ${includeDir}/proto )
file( MAKE_DIRECTORY ${protoIncludeDir} )

linkGeneratedHeader( ${targetName} "${PROJECT_SOURCE_DIR}/proto/Web.FromServer.pb.h" "${protoIncludeDir}/Web.FromServer.pb.h" )
linkGeneratedHeader( ${targetName} "${PROJECT_SOURCE_DIR}/proto/Web.FromServer.pb.h" "$ENV{JDE_DIR}/include/jde/app/proto/Web.FromServer.pb.h" )