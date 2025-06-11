cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( $ENV{JDE_DIR}/Public/build/functions.cmake )
project( Jde.Opc.Server VERSION ${JDE_VERSION} LANGUAGES CXX )

include( $ENV{JDE_DIR}/Public/build/dependencies.cmake )

set( targetName Jde.Opc.ServerLib )
add_library( ${targetName} )

file(GLOB sources *.cpp **/*.cpp )
get_filename_component( main_cpp ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp ABSOLUTE )
list( REMOVE_ITEM sources ${main_cpp} )
file(GLOB headers "**/*.h")
target_sources( "${targetName}" PRIVATE "${sources}" "${headers}" )

#include_directories( ${open62541Dir}/include )
# target_link_libraries( ${targetName} OpenSSL::SSL )
# #target_link_libraries( ${targetName} libcrypto.a)
# target_link_libraries( ${targetName} Boost::json )

# #target_link_libraries( ${targetName} debug libfmtd.a optimized libfmt.a )
# include_directories( ${spdlog_DIR}/../../../include )
# include_directories( ${protobuf_INCLUDE_DIRS} )
# include_directories( ${absl_DIR}/../../../include )
# include_directories( $ENV{JDE_DIR}/Public/include )
# target_link_libraries( ${targetName} Jde.Opc Jde.Crypto Jde )
# # target_link_libraries( ${targetName} Jde.Opc Jde.App.Client Jde.App.Shared Jde.Crypto Jde.Web.Client )
# target_link_libraries( ${targetName} Boost::json )

# target_link_libraries( ${targetName} protobuf::libprotobuf )
include_directories( ${open62541Dir}/include )
#target_link_libraries( ${targetName} stdc++exp )#for stacktrace

target_link_libraries( ${targetName} Jde.App.Client Jde.Web.Server Jde.Opc )
#target_link_libraries( ${targetName} stdc++_libbacktrace )
#--enable-libstdcxx-backtrace=yes, requires linking against -lstdc++_libbacktrace
#######################################################
target_precompile_headers( ${targetName}
	PRIVATE
		pc.h
)
