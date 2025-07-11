cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )

include( ../../build/functions.cmake )
project( Jde.Opc.Server VERSION ${JDE_VERSION} LANGUAGES CXX )

add_subdirectory( $ENV{JDE_DIR}/Framework/source ../../libs/framework )
add_subdirectory( $ENV{JDE_DIR}/Public/libs/crypto/src ../../libs/crypto )
set( opcCommonDir ../../libs/opc  )
add_subdirectory( ${opcCommonDir}/src ../../libs/opc/lib )
add_subdirectory( $ENV{JDE_DIR}/Public/libs/db/src ../../libs/db/lib )
if( WIN32 )
	add_subdirectory( $ENV{JDE_DIR}/Public/libs/db/drivers/odbc ../../libs/db/drivers/odbc )
else()
	add_subdirectory( $ENV{JDE_DIR}/Public/libs/db/drivers/mysql ../../libs/db/drivers/mysql )
endif()
add_subdirectory( $ENV{JDE_DIR}/Public/libs/ql ../../libs/ql )
add_subdirectory( $ENV{JDE_DIR}/Public/libs/access/src ../../libs/access/lib )
add_subdirectory( $ENV{JDE_DIR}/Public/libs/web/server ../../libs/web/server )
add_subdirectory( $ENV{JDE_DIR}/Public/libs/web/client ../../libs/web/client )
add_subdirectory( $ENV{JDE_DIR}/Public/libs/app/shared ../../libs/app/shared )
add_subdirectory( $ENV{JDE_DIR}/Public/libs/app/client ../../libs/app/client )

find_package( open62541 REQUIRED )
set( open62541Dir ${open62541_DIR}/../../.. )
cmake_path( ABSOLUTE_PATH open62541Dir NORMALIZE OUTPUT_VARIABLE open62541Dir )
link_directories( ${open62541Dir}/lib )
#add_definitions( -DUA_ENABLE_ENCRYPTION_OPENSSL )
include_directories( ${open62541Dir}/include )

add_subdirectory( src lib )

# if( buildTests )
# 	add_subdirectory( tests )
# 	set( CMAKE_CTEST_ARGUMENTS -c -settings=$ENV{JDE_DIR}/AppServer/config/App.Server.jsonnet )
# 	add_test( NAME AppTests COMMAND $<TARGET_FILE:Jde.App.Tests> )
# endif()
if( WIN32 )
	add_definitions( -DZLIB_USE_STATIC_LIBS -D )
	set( ZLIB_ROOT ${LIB_ROOT}/zlib )
endif()
find_package( ZLIB REQUIRED )

include( ../../build/dependencies.cmake )
set( targetName Jde.Opc.Server )
project( ${targetName} )

add_executable( ${targetName} )
compileOptions( ${targetName} )

set( sources src/main.cpp )
if( WIN32 )
	list( APPEND sources src/OpcServer.rc )
endif()
file( GLOB headers src/**/*.h )
target_sources( ${targetName} PRIVATE ${sources} ${headers} )

#include_directories( $ENV{JDE_DIR}/Public/include/jde/web/client/proto )
if( WIN32 )
	target_link_libraries( ${targetName} PRIVATE ZLIB::ZLIB )
	target_link_libraries( ${targetName} PRIVATE Iphlpapi.lib )
else()
	target_link_libraries( ${targetName} PRIVATE OpenSSL::SSL )
endif()
target_link_libraries( ${targetName} PRIVATE Boost::json )
target_link_libraries( ${targetName} PRIVATE protobuf::libprotobuf )
target_link_libraries( ${targetName} PRIVATE open62541 )
target_link_libraries( ${targetName} PRIVATE Jde Jde.DB Jde.QL Jde.Access Jde.Web.Server Jde.App.Client Jde.Opc Jde.Opc.ServerLib )
target_link_libraries( ${targetName} INTERFACE Jde.DB.MySql )

#add_custom_command( TARGET ${targetName} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink "../../libs/db/config/common-meta.libsonnet" "config/common-meta.libsonnet" COMMENT "mklink common-meta.libsonnet" )
if( WIN32 )
	set_target_properties( ${targetName} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>" )
	set_target_properties( ${targetName} PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "-c -tests -settings=${CMAKE_CURRENT_SOURCE_DIR}/config/Opc.Server.jsonnet" )
	copyCommonDlls()
endif()