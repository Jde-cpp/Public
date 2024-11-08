cmake_minimum_required(VERSION 3.20.0)
set( CMAKE_MODULE_PATH $ENV{JDE_DIR}/Public/build )
include(common)
findBoost()

if( ${VCPKG} )
	SET(CMAKE_TOOLCHAIN_FILE "$ENV{REPO_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"  CACHE STRING "Vcpkg toolchain file")
endif()
project(Jde.App.Client)
include(compiler)

find_package( OpenSSL REQUIRED )
#dump_cmake_variables()
if(MSVC)
	include_directories( ${OPENSSL_INCLUDE_DIR} )
	find_package( Protobuf REQUIRED )
	find_package( fmt )
	link_directories( $ENV{JDE_DIR}/Framework/source/.build/.bin/${outDir} )
	link_directories( $ENV{JDE_DIR}/Public/src/app/shared/.build/.bin/${outDir} )
	link_directories( $ENV{JDE_DIR}/Public/src/crypto/.build/.bin/${outDir} )
	link_directories( $ENV{JDE_DIR}/Public/src/web/client/.build/.bin/${outDir} )
else()
	link_directories( ${Protobuf_DIR}/../.. )
endif()

#######################################################
add_library( ${CMAKE_PROJECT_NAME} SHARED )
#set( CMAKE_INCLUDE_CURRENT_DIR ON )
file(GLOB src_files *.cpp await/*.cpp )
file(GLOB hdr_files  **/*.h )
target_sources( "${CMAKE_PROJECT_NAME}"
    PRIVATE
        "${src_files}"
        "${hdr_files}")

if(MSVC)
	target_link_libraries( ${CMAKE_PROJECT_NAME} Jde Jde.App.Shared Jde.Web.Client )
	target_link_libraries( ${CMAKE_PROJECT_NAME} fmt::fmt )
	target_link_libraries( ${CMAKE_PROJECT_NAME} protobuf::libprotobuf )
else()
	add_definitions( -DJde_App_Client_EXPORTS )
	target_link_libraries( ${CMAKE_PROJECT_NAME} Threads::Threads )
	target_link_libraries( ${CMAKE_PROJECT_NAME} debug libfmtd.a optimized libfmt.a )
	target_link_libraries( ${CMAKE_PROJECT_NAME} protobuf::libprotobuf libutf8_range.a )
endif()
target_link_libraries( ${CMAKE_PROJECT_NAME} OpenSSL::SSL )
#######################################################
target_precompile_headers(${CMAKE_PROJECT_NAME}
  PRIVATE
	pc.h
)
if( MSVC )
	set( CMAKE_INSTALL_PREFIX $ENV{JDE_DIR}/installed )
	set( installDir ${CMAKE_INSTALL_PREFIX}/$<CONFIGURATION> )
	set( configStem jdeAppClientConfig )
	install( TARGETS ${CMAKE_PROJECT_NAME}
		EXPORT ${configStem}
		RUNTIME DESTINATION ${installDir}/bin
		ARCHIVE DESTINATION ${installDir}/lib )
	install( FILES $<TARGET_PDB_FILE:${CMAKE_PROJECT_NAME}> CONFIGURATIONS Debug RelWithDebInfo DESTINATION ${installDir}/bin )
	export( TARGETS ${CMAKE_PROJECT_NAME} NAMESPACE Jde:: FILE ${CMAKE_CURRENT_BINARY_DIR}/${configStem}.cmake )
	install( EXPORT ${configStem} DESTINATION ${CMAKE_INSTALL_PREFIX}/cmake NAMESPACE Jde:: )
endif()