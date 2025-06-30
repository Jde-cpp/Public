cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( ../../../build/functions.cmake )
message( Jde.Web.Client)
project( Jde.Web.Client VERSION ${JDE_VERSION} LANGUAGES CXX )
include( ../../../build/dependencies.cmake )

if( WIN32 )
	add_definitions( -DZLIB_USE_STATIC_LIBS -D )
	if( ${LIB_DIR} STREQUAL "" )
		message( FATAL_ERROR "Set LIB_ROOT cmake variable" )
	endif()
	set( ZLIB_ROOT ${LIB_DIR}/zlib )
endif()
find_package( ZLIB REQUIRED )
include_directories( ${ZLIB_INCLUDE_DIRS} )

set( targetName Jde.Web.Client )
add_library( ${targetName} )
compileOptions( ${targetName} )
if( WIN32 )
	target_compile_options( ${targetName} PRIVATE /bigobj )
endif()
set( CMAKE_INCLUDE_CURRENT_DIR ON )
file( GLOB sources *.cpp http/*.cpp socket/*.cpp )
set( includeDir $ENV{JDE_DIR}/Public/include/jde/web/client )
file( GLOB headers *.h ${includeDir}/*.h ${includeDir}/http/*.h ${includeDir}/socket/*.h  )
file( GLOB protos proto/*.proto )
target_sources( ${targetName} PRIVATE ${sources} ${headers} ${protos} )

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
set( protoIncludeDir ${includeDir}/proto )
file( MAKE_DIRECTORY ${protoIncludeDir} )

add_custom_command( TARGET ${targetName} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink "${PROJECT_SOURCE_DIR}/proto/Web.FromServer.pb.h" "${protoIncludeDir}/Web.FromServer.pb.h" COMMENT "mklink web/client/proto/Web.FromServer.pb.h" )
add_custom_command( TARGET ${targetName} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink "${PROJECT_SOURCE_DIR}/proto/Web.FromServer.pb.h" "$ENV{JDE_DIR}/Public/include/jde/app/shared/proto/Web.FromServer.pb.h" COMMENT "mklink jde/app/shared/proto/Web.FromServer.pb.h" )
#add_custom_command( TARGET ${targetName} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink "${PROJECT_SOURCE_DIR}/proto/Web.FromServer.proto" "$ENV{JDE_DIR}/Public/libs/app/shared/proto/Web.FromServer.proto" COMMENT "mklink libs/app/shared/Web.FromServer.proto" )