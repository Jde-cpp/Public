cmake_minimum_required(VERSION 3.20.0)
set( CMAKE_MODULE_PATH $ENV{JDE_DIR}/Public/build )
include(common)
findBoost()
if( ${VCPKG} )
	SET(CMAKE_TOOLCHAIN_FILE "$ENV{REPO_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"  CACHE STRING "Vcpkg toolchain file")
endif()
project(Jde.DB)
include(compiler)
#######################################################
if( MSVC )
	link_directories( $ENV{JDE_DIR}/Framework/source/.build/.bin/${outDir} )
else()
	link_directories( ${fmt_DIR}/../.. )
	link_directories( ${Protobuf_DIR}/../.. )
	add_definitions( -DJde_QL_EXPORTS )
endif()
add_library( ${CMAKE_PROJECT_NAME} SHARED )
#######################################################
file(GLOB sources await/*.cpp generators/*.cpp *.cpp meta/*.cpp meta/ddl/*.cpp )
file(GLOB headers *.h ../../jde/db/*.h ../../jde/db/meta/*.h ../../jde/db/syntax/*.h )
target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${sources}
        ${headers}
)
#######################################################
if(MSVC)
else()
#	add_definitions( -DJde_Crypto_EXPORTS )
	target_link_libraries( ${CMAKE_PROJECT_NAME} Threads::Threads )
	#target_link_libraries( ${CMAKE_PROJECT_NAME} debug libfmtd.a optimized libfmt.a )
	#target_link_libraries( ${CMAKE_PROJECT_NAME} libcrypto.a )
endif()
	target_link_libraries( ${CMAKE_PROJECT_NAME} fmt::fmt )
#######################################################

target_precompile_headers(${CMAKE_PROJECT_NAME}
  PRIVATE
  <jde/framework.h>
)
