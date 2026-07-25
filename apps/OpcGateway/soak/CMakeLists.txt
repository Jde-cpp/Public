cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( ${CMAKE_CURRENT_LIST_DIR}/../../../build/functions.cmake )
project( Jde.Opc.Soak VERSION ${JDE_VERSION} LANGUAGES CXX )
include( ${jdeRoot}/build/dependencies.cmake )

find_package( open62541 REQUIRED )
add_definitions( -DUA_ENABLE_ENCRYPTION )
if( WIN32 )
	add_definitions( -DZLIB_USE_STATIC_LIBS )
	set( ZLIB_ROOT ${CMAKE_INSTALL_PREFIX}/zlib )
else()
	if( NOT CMAKE_BUILD_TYPE STREQUAL "Debug" )
		link_directories( ${JSONNET_DIR}/lib )
	endif()
endif()
find_package( ZLIB REQUIRED )

set( targetName Jde.Opc.Soak )
add_executable( ${targetName} )
compileOptions( ${targetName} )

#GatewayClientSocket comes from the test tree - the wire-protocol driver is shared, but no GTest and no embedded servers here.
target_sources( ${targetName} PRIVATE main.cpp SoakAppClient.cpp SoakRunner.cpp ../tests/utils/GatewayClientSocket.cpp )

target_include_directories( ${targetName} PRIVATE ${jdeRoot}/include/jde/app/proto )

target_link_libraries( ${targetName} Jde Jde.QL Jde.App.Shared Jde.App.Client Jde.Web.Client Jde.Web.Server Jde.Opc Jde.Opc.GatewayLib )
if( WIN32 )
	target_link_libraries( ${targetName} Iphlpapi.lib )
	copyCommonDlls()
	target_compile_definitions( ${targetName} PRIVATE NOMINMAX )
	add_custom_command( TARGET ${targetName} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_INSTALL_PREFIX}/libxml2/bin/libxml2.dll" $<TARGET_FILE_DIR:${targetName}>  COMMENT "libxml2.dll" )
else()
	target_link_libraries( ${targetName} OpenSSL::SSL )
	if( NOT CMAKE_BUILD_TYPE STREQUAL "Debug" )
		target_link_libraries( ${targetName} libjsonnet++.so libjsonnet.so )
	endif()
endif()
target_precompile_headers( ${targetName} PRIVATE
	pc.h
)
