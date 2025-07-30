cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( ../../../build/functions.cmake )
project( Jde.App.Shared VERSION ${JDE_VERSION} LANGUAGES CXX )
include( ../../../build/dependencies.cmake )

set( targetName Jde.App.Shared )
add_library( ${targetName} )
compileOptions( ${targetName} )
target_include_directories( ${targetName} PRIVATE $ENV{JDE_DIR}/Public/include/jde/web/client/proto )

file(GLOB sources *.cpp proto/*.cpp )
file(GLOB headers  **/*.h $ENV{JDE_DIR}/Public/include/jde/app/shared/*.h )
file(GLOB protos proto/*.proto )
target_sources( ${targetName} PRIVATE ${sources} ${headers} ${protos} )

#######################################################
target_precompile_headers(${targetName}
  PUBLIC
	<jde/web/client/exports.h>
	<jde/app/shared/exports.h>
	<jde/web/client/proto/Web.FromServer.pb.h>
	<jde/framework.h>
	PRIVATE
	proto/Common.pb.h
	proto/App.pb.h
	proto/App.FromServer.pb.h
	proto/App.FromClient.pb.h
)

set( protoOutDir ${PROJECT_SOURCE_DIR}/proto )
protobuf_generate(
	TARGET ${targetName}
	IMPORT_DIRS proto ../../web/client/proto
	PROTOC_OUT_DIR ${protoOutDir}
	EXPORT_MACRO Jde_App_Shared_EXPORT
)

add_custom_command( TARGET ${targetName} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink "${protoOutDir}/App.FromServer.pb.h" "$ENV{JDE_DIR}/Public/include/jde/app/shared/proto/App.FromServer.pb.h" COMMENT "mklink FromServer" )
add_custom_command( TARGET ${targetName} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink "${protoOutDir}/App.FromClient.pb.h" "$ENV{JDE_DIR}/Public/include/jde/app/shared/proto/App.FromClient.pb.h" COMMENT "mklink FromClient" )
add_custom_command( TARGET ${targetName} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink "${protoOutDir}/Common.pb.h" "$ENV{JDE_DIR}/Public/include/jde/app/shared/proto/Common.pb.h" COMMENT "mklink Common" )
#add_custom_command( TARGET ${targetName} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink "${protoOutDir}/Common.pb.h" "$ENV{JDE_DIR}/Public/include/jde/opc/types/proto/Common.pb.h" COMMENT "mklink Common" )
add_custom_command( TARGET ${targetName} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink "${protoOutDir}/App.pb.h" "$ENV{JDE_DIR}/Public/include/jde/app/shared/proto/App.pb.h" COMMENT "mklink App" )
#add_custom_command( TARGET ${targetName} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink "${protoOutDir}/Common.proto" "$ENV{JDE_DIR}/Public/libs/opc/src/types/proto/Common.proto" COMMENT "mklink Common->iot" )