boost()

find_package( OpenSSL REQUIRED )

find_package( fmt REQUIRED )
include_directories( ${fmt_DIR}/../../../include )

find_package( spdlog REQUIRED )
include_directories( ${spdlog_DIR}/../../../include )
set( LIB_DIR $ENV{LIB_DIR} CACHE PATH "Path to libraries" )
list( PREPEND CMAKE_PREFIX_PATH ${LIB_DIR}/protobuf/lib/cmake/utf8_range )
find_package( protobuf CONFIG "25.6.0" EXACT REQUIRED )
get_filename_component( protobuf_INCLUDE_DIRS ${protobuf_DIR}/../../../include ABSOLUTE )
include_directories( ${protobuf_INCLUDE_DIRS} )
include_directories( ${absl_DIR}/../../../include )

include_directories( $ENV{LIB_DIR}/jsonnet/include )

include_directories( $ENV{JDE_DIR}/Public/include )
