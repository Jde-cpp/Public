cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( ../../../build/functions.cmake )
project( Jde.App.Shared VERSION ${JDE_VERSION} LANGUAGES CXX )
include( ../../../build/dependencies.cmake )

set( targetName Jde.App.Shared )
add_library( ${targetName} )
compileOptions( ${targetName} )
target_include_directories( ${targetName} PRIVATE $ENV{JDE_DIR}/include/jde/web/client/proto )

file(GLOB sources *.cpp proto/*.cpp log/*.cpp )
file(GLOB headers  **/*.h $ENV{JDE_DIR}/include/jde/app/shared/*.h )
file(GLOB protos proto/*.proto )
target_sources( ${targetName} PRIVATE ${sources} ${headers} ${protos} )

#######################################################
target_precompile_headers(${targetName}
  PUBLIC
	<jde/web/client/exports.h>
	<jde/web/client/proto/Web.FromServer.pb.h>
	<jde/fwk.h>
	PRIVATE
	pc.h
)

set( protoOutDir ${PROJECT_SOURCE_DIR}/proto )
protobuf_generate(
	TARGET ${targetName}
	IMPORT_DIRS proto ../../web/client/proto
	PROTOC_OUT_DIR ${protoOutDir}
)

add_custom_command( TARGET ${targetName} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink "${protoOutDir}/App.FromServer.pb.h" "$ENV{JDE_DIR}/include/jde/app/proto/App.FromServer.pb.h" COMMENT "mklink FromServer" )
add_custom_command( TARGET ${targetName} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink "${protoOutDir}/App.FromClient.pb.h" "$ENV{JDE_DIR}/include/jde/app/proto/App.FromClient.pb.h" COMMENT "mklink FromClient" )
add_custom_command( TARGET ${targetName} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink "${protoOutDir}/Common.pb.h" "$ENV{JDE_DIR}/include/jde/app/proto/Common.pb.h" COMMENT "mklink Common" )
add_custom_command( TARGET ${targetName} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink "${protoOutDir}/App.pb.h" "$ENV{JDE_DIR}/include/jde/app/proto/App.pb.h" COMMENT "mklink App" )
add_custom_command( TARGET ${targetName} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink "${protoOutDir}/Log.pb.h" "$ENV{JDE_DIR}/include/jde/app/proto/Log.pb.h" COMMENT "mklink Log" )