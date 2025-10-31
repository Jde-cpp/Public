cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )

include( $ENV{JDE_DIR}/Public/build/functions.cmake )
project( Jde.App.Server VERSION ${JDE_VERSION} LANGUAGES CXX )

add_subdirectory( $ENV{JDE_DIR}/Public/libs/fwk/src ../../libs/fwk/lib )
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

add_subdirectory( src lib )

if( WIN32 )
	add_definitions( -DZLIB_USE_STATIC_LIBS -D )
	set( ZLIB_ROOT ${LIB_ROOT}/zlib )
endif()
find_package( ZLIB REQUIRED )

include( $ENV{JDE_DIR}/Public/build/dependencies.cmake )
set( targetName Jde.App.Server )
project( ${targetName} )

add_executable( ${targetName} )
compileOptions( ${targetName} )

set( sources src/main.cpp )
if( WIN32 )

	list( APPEND sources src/AppServer.rc )
endif()
file( GLOB headers src/**/*.h )
target_sources( ${targetName} PRIVATE ${sources} ${headers} )

include_directories( $ENV{JDE_DIR}/Public/include/jde/web/client/proto )
if( WIN32 )
	target_link_libraries( ${targetName} ZLIB::ZLIB )
	set_target_properties( ${targetName} PROPERTIES LINK_FLAGS ${CMAKE_EXE_LINKER_FLAGS} )
else()
	target_link_libraries( ${targetName} OpenSSL::SSL )
endif()
target_link_libraries( ${targetName} Boost::json )
target_link_libraries( ${targetName} protobuf::libprotobuf )

target_link_libraries( ${targetName} Jde Jde.QL Jde.Web.Server Jde.App.ServerLib )

add_custom_command( TARGET ${targetName} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink "$ENV{JDE_DIR}/Public/libs/db/config/common-meta.libsonnet" "$ENV{JDE_DIR}/Public/apps/AppServer/config/common-meta.libsonnet" COMMENT "mklink common-meta.libsonnet" )
if( WIN32 )
	set_target_properties( ${targetName} PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "-c -tests -settings=${CMAKE_CURRENT_SOURCE_DIR}/config/App.Server.jsonnet" )
	copyCommonDlls()
endif()