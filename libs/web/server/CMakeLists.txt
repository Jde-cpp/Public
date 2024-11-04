cmake_minimum_required(VERSION 3.20)
set( CMAKE_MODULE_PATH $ENV{JDE_DIR}/Public/build )
include(common)
findBoost()
if( ${VCPKG} )
	SET( CMAKE_TOOLCHAIN_FILE $ENV{REPO_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake  CACHE STRING "Vcpkg toolchain file" )
endif()
project( Jde.Web.Server )
include( compiler )

find_package( OpenSSL )
if( MSVC )
	include_directories( ${OPENSSL_INCLUDE_DIR} )
	link_directories( $ENV{JDE_DIR}/Framework/source/.build/.bin/${outDir} )
	link_directories( $ENV{JDE_DIR}/Public/src/crypto/.build/.bin/${outDir} )
	link_directories( $ENV{JDE_DIR}/Public/src/app/shared/.build/.bin/${outDir} )
	string( APPEND CMAKE_CXX_FLAGS " /bigobj " )
else()
	link_directories( ${Protobuf_DIR}/../.. )
	add_definitions( -DJde_Web_Server_EXPORTS )
endif()
#######################################################
add_library( ${CMAKE_PROJECT_NAME} SHARED )
file(GLOB sources *.cpp )
file(GLOB headers   *.h ../../../jde/web/server/*.h )
target_sources( ${CMAKE_PROJECT_NAME} PRIVATE ${sources} ${headers} )

if(MSVC)
	find_package( fmt REQUIRED )
	find_package( ZLIB )
	find_package( Protobuf REQUIRED )

	target_link_libraries( ${CMAKE_PROJECT_NAME} Jde )
	target_link_libraries( ${CMAKE_PROJECT_NAME} fmt::fmt )
	target_link_libraries( ${CMAKE_PROJECT_NAME} ZLIB::ZLIB )
	target_link_libraries( ${CMAKE_PROJECT_NAME} protobuf::libprotobuf )
else()
	target_link_libraries( ${CMAKE_PROJECT_NAME} Threads::Threads )
	target_link_libraries( ${CMAKE_PROJECT_NAME} debug libfmtd.a optimized libfmt.a )
	target_link_libraries( ${CMAKE_PROJECT_NAME} protobuf::libprotobuf libutf8_range.a )# libutf8_validity.a )
endif()
target_link_libraries( ${CMAKE_PROJECT_NAME} OpenSSL::SSL )
#######################################################
target_precompile_headers( ${CMAKE_PROJECT_NAME}
  PRIVATE
	"pc.h"
)

if( MSVC )
	set( CMAKE_INSTALL_PREFIX $ENV{JDE_DIR}/installed )
	set( installDir ${CMAKE_INSTALL_PREFIX}/$<CONFIGURATION> )
	set( configStem jdeWebServerConfig )
	install( TARGETS ${CMAKE_PROJECT_NAME}
		EXPORT ${configStem}
		RUNTIME DESTINATION ${installDir}/bin
		ARCHIVE DESTINATION ${installDir}/lib )
	install( FILES $<TARGET_PDB_FILE:${CMAKE_PROJECT_NAME}> CONFIGURATIONS Debug RelWithDebInfo DESTINATION ${installDir}/bin )
	export( TARGETS ${CMAKE_PROJECT_NAME} NAMESPACE Jde:: FILE ${CMAKE_CURRENT_BINARY_DIR}/${configStem}.cmake )
	install( EXPORT ${configStem} DESTINATION ${CMAKE_INSTALL_PREFIX}/cmake NAMESPACE Jde:: )
endif()