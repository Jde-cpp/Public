boost()
find_package( OpenSSL REQUIRED )

find_package( fmt REQUIRED ) #no include_directories climb: Jde exports fmt::fmt PUBLIC, leaf targets link fmt::fmt themselves.

find_package( spdlog REQUIRED )
add_compile_definitions( SPDLOG_FMT_EXTERNAL )
include_directories( ${spdlog_DIR}/../../../include )
list( APPEND CMAKE_PREFIX_PATH "${CMAKE_INSTALL_PREFIX}/protobuf/lib/cmake/utf8_range" )

find_package( protobuf CONFIG "34.1.0" EXACT REQUIRED )
find_package( absl CONFIG REQUIRED ) #absl_DIR was previously only set as a side effect of protobuf's find_dependency.
get_filename_component( protobuf_INCLUDE_DIRS ${protobuf_DIR}/../../../include ABSOLUTE )
include_directories( SYSTEM ${protobuf_INCLUDE_DIRS} )
include_directories( SYSTEM ${absl_DIR}/../../../include )
include_directories( ${CMAKE_CURRENT_LIST_DIR}/../include )
