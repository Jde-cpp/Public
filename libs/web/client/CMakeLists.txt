cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( ${CMAKE_CURRENT_LIST_DIR}/../../../build/functions.cmake )
message( Jde.Web.Client)
project( Jde.Web.Client VERSION ${JDE_VERSION} LANGUAGES CXX )
include( ${jdeRoot}/build/dependencies.cmake )

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
set( includeDir ${jdeRoot}/include/jde/web/client )
file( GLOB headers *.h ${includeDir}/*.h ${includeDir}/http/*.h ${includeDir}/socket/*.h  )
file( GLOB protos proto/*.proto )
target_sources( ${targetName} PRIVATE ${sources} ${headers} ${protos} )

target_link_libraries( ${targetName} PUBLIC ZLIB::ZLIB Boost::json OpenSSL::SSL fmt::fmt )

target_precompile_headers(${targetName}
  PUBLIC
	"pc.h"
)

#Out-of-source - see the note in libs/app/shared/CMakeLists.txt.
set( protoOutDir ${CMAKE_CURRENT_BINARY_DIR}/proto )
file( MAKE_DIRECTORY ${protoOutDir} )
protobuf_generate(
	TARGET ${targetName}
	IMPORT_DIRS proto
	PROTOC_OUT_DIR ${protoOutDir}
	EXPORT_MACRO Jde_Web_Client_EXPORT
)
suppressProtoWarnings( "${protos}" ${protoOutDir} )
target_include_directories( ${targetName} PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${protoOutDir} )
set( protoIncludeDir ${includeDir}/proto )
file( MAKE_DIRECTORY ${protoIncludeDir} )

linkGeneratedHeader( ${targetName} "${protoOutDir}/Web.FromServer.pb.h" "${protoIncludeDir}/Web.FromServer.pb.h" )
linkGeneratedHeader( ${targetName} "${protoOutDir}/Web.FromServer.pb.h" "${jdeRoot}/include/jde/app/proto/Web.FromServer.pb.h" )