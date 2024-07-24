cmake_minimum_required(VERSION 3.16)
project(Jde.App.Shared)
set( CMAKE_MODULE_PATH $ENV{JDE_DIR}/Public/build )
include(common)
link_directories( ${Protobuf_DIR}/../.. )
#######################################################
set(CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS} "-Wl,-rpath=$ORIGIN")
add_definitions( -DJde_APP_CLIENT_EXPORTS )
find_package(OpenSSL)
add_library( ${CMAKE_PROJECT_NAME} SHARED )
set( CMAKE_INCLUDE_CURRENT_DIR ON )
file(GLOB src_files *.cpp proto/*.cpp )
file(GLOB hdr_files  **/*.h )
file(GLOB proto_files proto/*.proto )
target_sources( "${CMAKE_PROJECT_NAME}"
    PRIVATE
        "${src_files}"
        "${hdr_files}"
        "${proto_files}")

target_link_libraries( ${CMAKE_PROJECT_NAME} debug libfmtd.a optimized libfmt.a )
target_link_libraries( ${CMAKE_PROJECT_NAME} protobuf::libprotobuf libutf8_range.a )
if(MSVC)
	add_definitions( -DWIN32_LEAN_AND_MEAN )
else()
	target_link_libraries(${CMAKE_PROJECT_NAME} Threads::Threads)
endif()
#######################################################
target_precompile_headers(${CMAKE_PROJECT_NAME}
  PRIVATE
	#<boost/beast.hpp>
	#<boost/asio.hpp>
	#<boost/beast/ssl/ssl_stream.hpp>
	<Common.pb.h>
	<App.pb.h>
	<App.FromServer.pb.h>
	<App.FromClient.pb.h>
	<jde/TypeDefs.h>
	<jde/log/Log.h>
	<jde/Exception.h>
)

protobuf_generate(
	TARGET "${CMAKE_PROJECT_NAME}"
	IMPORT_DIRS proto )

add_custom_command( TARGET ${CMAKE_PROJECT_NAME} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink "${CMAKE_BINARY_DIR}//App.FromServer.pb.h" "$ENV{JDE_DIR}/Public/jde/app/shared/proto/App.FromServer.pb.h" COMMENT "mklink FromServer" )
add_custom_command( TARGET ${CMAKE_PROJECT_NAME} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink "${CMAKE_BINARY_DIR}/App.FromClient.pb.h" "$ENV{JDE_DIR}/Public/jde/app/shared/proto/App.FromClient.pb.h" COMMENT "mklink FromClient" )
add_custom_command( TARGET ${CMAKE_PROJECT_NAME} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink "${CMAKE_BINARY_DIR}/Common.pb.h" "$ENV{JDE_DIR}/Public/jde/app/shared/proto/Common.pb.h" COMMENT "mklink Common" )
add_custom_command( TARGET ${CMAKE_PROJECT_NAME} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink "${CMAKE_BINARY_DIR}/App.pb.h" "$ENV{JDE_DIR}/Public/jde/app/shared/proto/App.pb.h" COMMENT "mklink App" )
add_custom_command( TARGET ${CMAKE_PROJECT_NAME} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink "${CMAKE_CURRENT_SOURCE_DIR}/proto/Common.proto" "$ENV{JDE_DIR}/Public/src/iot/types/proto/Common.proto" COMMENT "mklink Common->iot" )