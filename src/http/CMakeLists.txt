cmake_minimum_required(VERSION 3.16)
project(Jde.Http)
set( CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../../build )
include(common)
link_directories( ${Protobuf_DIR}/../.. )
#######################################################
set(CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS} "-Wl,-rpath=$ORIGIN")
add_definitions( -DJde_HTTP_EXPORTS )
find_package(OpenSSL)

add_library( ${CMAKE_PROJECT_NAME} SHARED )
file(GLOB src_files   "*.cpp")
file(GLOB hdr_files   "*.h")
file(GLOB proto_files   "**/*.proto")

set(CMAKE_INCLUDE_CURRENT_DIR ON)
message( "src_files=${src_files}" )
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
	<boost/beast.hpp>
	<boost/asio.hpp>
	<boost/beast/ssl.hpp>
	<jde/TypeDefs.h>
)
protobuf_generate( TARGET "${CMAKE_PROJECT_NAME}" IMPORT_DIRS proto )