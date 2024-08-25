cmake_minimum_required(VERSION 3.20.0)
set( CMAKE_MODULE_PATH $ENV{JDE_DIR}/Public/build )
include(common)
findBoost()
if( ${VCPKG} )
	SET(CMAKE_TOOLCHAIN_FILE "$ENV{REPO_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"  CACHE STRING "Vcpkg toolchain file")
endif()
project(Jde.App.Shared)

include(compiler)

if(MSVC)
find_package( Protobuf REQUIRED )
find_package( fmt )
#set( Jde_DIR $ENV{JDE_DIR}/Framework/source )
#find_package(Jde)
	link_directories( $ENV{JDE_DIR}/Framework/source/.build/.bin/${outDir} )
else()
	link_directories( ${Protobuf_DIR}/../.. )
endif()
#######################################################

find_package(OpenSSL)
add_library( ${CMAKE_PROJECT_NAME} SHARED )
set( CMAKE_INCLUDE_CURRENT_DIR ON )
#message( "CMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}" )
file(GLOB src_files *.cpp proto/*.cpp )
file(GLOB hdr_files  **/*.h )
file(GLOB proto_files proto/*.proto )
target_sources( "${CMAKE_PROJECT_NAME}"
    PRIVATE
        "${src_files}"
        "${hdr_files}"
        "${proto_files}")

if(MSVC)
	target_link_libraries( ${CMAKE_PROJECT_NAME} fmt::fmt )
	target_link_libraries( ${CMAKE_PROJECT_NAME} protobuf::libprotobuf )
	target_link_libraries( ${CMAKE_PROJECT_NAME} Jde )
else()
	target_link_libraries( ${CMAKE_PROJECT_NAME} Threads::Threads )
	target_link_libraries( ${CMAKE_PROJECT_NAME} debug libfmtd.a optimized libfmt.a )
	target_link_libraries( ${CMAKE_PROJECT_NAME} protobuf::libprotobuf libutf8_range.a )
endif()


if(MSVC)
	string( REPLACE "/W4" "/W0" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG} )
#	message( CMAKE_CXX_FLAGS_DEBUG=${CMAKE_CXX_FLAGS_DEBUG} )
	set_source_files_properties(
		${src_files}
		PROPERTIES
		COMPILE_FLAGS "/W4"
	)
endif()
#######################################################
target_precompile_headers(${CMAKE_PROJECT_NAME}
  PRIVATE
	<jde/app/shared/exports.h>
	<Common.pb.h>
	<App.pb.h>
	<App.FromServer.pb.h>
	<App.FromClient.pb.h>
	<jde/TypeDefs.h>
	<jde/log/Log.h>
	<jde/Exception.h>
)

protobuf_generate( TARGET "${CMAKE_PROJECT_NAME}" IMPORT_DIRS proto EXPORT_MACRO Jde_App_Shared_EXPORT )

add_custom_command( TARGET ${CMAKE_PROJECT_NAME} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink "${CMAKE_BINARY_DIR}//App.FromServer.pb.h" "$ENV{JDE_DIR}/Public/jde/app/shared/proto/App.FromServer.pb.h" COMMENT "mklink FromServer" )
add_custom_command( TARGET ${CMAKE_PROJECT_NAME} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink "${CMAKE_BINARY_DIR}/App.FromClient.pb.h" "$ENV{JDE_DIR}/Public/jde/app/shared/proto/App.FromClient.pb.h" COMMENT "mklink FromClient" )
add_custom_command( TARGET ${CMAKE_PROJECT_NAME} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink "${CMAKE_BINARY_DIR}/Common.pb.h" "$ENV{JDE_DIR}/Public/jde/app/shared/proto/Common.pb.h" COMMENT "mklink Common" )
add_custom_command( TARGET ${CMAKE_PROJECT_NAME} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink "${CMAKE_BINARY_DIR}/App.pb.h" "$ENV{JDE_DIR}/Public/jde/app/shared/proto/App.pb.h" COMMENT "mklink App" )
add_custom_command( TARGET ${CMAKE_PROJECT_NAME} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink "${CMAKE_CURRENT_SOURCE_DIR}/proto/Common.proto" "$ENV{JDE_DIR}/Public/src/iot/types/proto/Common.proto" COMMENT "mklink Common->iot" )