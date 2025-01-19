cmake_minimum_required(VERSION 3.20)
set( CMAKE_MODULE_PATH $ENV{JDE_DIR}/Public/build )
include(common)
findBoost()
if( ${VCPKG} )
	SET( CMAKE_TOOLCHAIN_FILE $ENV{REPO_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake CACHE STRING "Vcpkg toolchain file" )
endif()

project( Jde.Web.Client )
include( compiler )

find_package( OpenSSL REQUIRED )
if( MSVC )
	include_directories( ${OPENSSL_INCLUDE_DIR} )
	link_directories( $ENV{JDE_DIR}/Framework/source/.build/.bin/${outDir} )
	link_directories( $ENV{JDE_DIR}/Public/src/crypto/.build/.bin/${outDir} )
	string( APPEND CMAKE_CXX_FLAGS " /bigobj " )
else()
	link_directories( ${Protobuf_DIR}/../.. )
	add_definitions( -DJde_WEB_CLIENT_EXPORTS )
endif()
#######################################################

add_library( ${CMAKE_PROJECT_NAME} )

set( CMAKE_INCLUDE_CURRENT_DIR ON )
file( GLOB sources *.cpp http/*.cpp socket/*.cpp )
set( includeDir $ENV{JDE_DIR}/Public/include/jde/web/client )
file( GLOB headers *.h ${includeDir}/*.h ${includeDir}/http/*.h ${includeDir}/socket/*.h  )
file( GLOB protos proto/*.proto )

target_sources( ${CMAKE_PROJECT_NAME} PRIVATE ${sources} ${headers} ${protos} )

if( MSVC )
	find_package( fmt )
	find_package( ZLIB )

	target_link_libraries( ${CMAKE_PROJECT_NAME} Jde )
	target_link_libraries( ${CMAKE_PROJECT_NAME} fmt::fmt )
	target_link_libraries( ${CMAKE_PROJECT_NAME} ZLIB::ZLIB )
	target_link_libraries( ${CMAKE_PROJECT_NAME} OpenSSL::SSL )
else()
#	target_link_libraries( ${CMAKE_PROJECT_NAME} debug libfmtd.a optimized libfmt.a )
#	target_link_libraries( ${CMAKE_PROJECT_NAME} protobuf::libprotobuf libutf8_range.a )
#	target_link_libraries( ${CMAKE_PROJECT_NAME} Threads::Threads )
endif()

#######################################################
target_precompile_headers(${CMAKE_PROJECT_NAME}
  PRIVATE
	"pc.h"
)

protobuf_generate( TARGET ${CMAKE_PROJECT_NAME} IMPORT_DIRS proto EXPORT_MACRO Jde_Web_Client_EXPORT )
set( protoIncludeDir ${includeDir}/proto )
file(MAKE_DIRECTORY ${protoIncludeDir})
add_custom_command( TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink "${CMAKE_BINARY_DIR}/Web.FromServer.pb.h" "${protoIncludeDir}/Web.FromServer.pb.h" COMMENT "mklink web/client/proto/Web.FromServer.pb.h" )
add_custom_command( TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink "${PROJECT_SOURCE_DIR}/proto/Web.FromServer.proto" "$ENV{JDE_DIR}/Public/libs/app/shared/proto/Web.FromServer.proto" COMMENT "mklink libs/app/shared/Web.FromServer.proto" )



if( MSVC )
	set( CMAKE_INSTALL_PREFIX $ENV{JDE_DIR}/installed )
	set( installDir ${CMAKE_INSTALL_PREFIX}/$<CONFIGURATION> )
	set( configStem jdeWebClientConfig )
	install( TARGETS ${CMAKE_PROJECT_NAME}
		EXPORT ${configStem}
		RUNTIME DESTINATION ${installDir}/bin
		ARCHIVE DESTINATION ${installDir}/lib )
	install( FILES $<TARGET_PDB_FILE:${CMAKE_PROJECT_NAME}> CONFIGURATIONS Debug RelWithDebInfo DESTINATION ${installDir}/bin )
	export( TARGETS ${CMAKE_PROJECT_NAME} NAMESPACE Jde:: FILE ${CMAKE_CURRENT_BINARY_DIR}/${configStem}.cmake )
	install( EXPORT ${configStem} DESTINATION ${CMAKE_INSTALL_PREFIX}/cmake NAMESPACE Jde:: )
endif()