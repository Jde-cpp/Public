cmake_minimum_required(VERSION 3.20)
set( CMAKE_MODULE_PATH $ENV{JDE_DIR}/Public/build )
include(common)
findBoost()
if( ${VCPKG} )
	SET( CMAKE_TOOLCHAIN_FILE $ENV{REPO_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake  CACHE STRING "Vcpkg toolchain file" )
endif()
project( Jde.Crypto.Tests VERSION 1.0.0.0 )
include( compiler )
if( NOT ${VCPKG} )
	include( tests )
endif()
#find_package( OpenSsl REQUIRED )
find_package( GTest REQUIRED )
if( MSVC )
	include_directories( ${OPENSSL_INCLUDE_DIR} )
	link_directories( $ENV{JDE_DIR}/Framework/source/.build/.bin )
	link_directories( $ENV{JDE_DIR}/Public/src/crypto/.build/.bin )
else()
	link_directories( ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} )
	link_directories( ${fmt_DIR}/../.. )
	link_directories( ${Protobuf_DIR}/../.. )
	link_directories( $ENV{REPO_DIR}/install/g++-13/asan/jsonnet/lib/ )
endif()

add_executable( ${CMAKE_PROJECT_NAME} )
file( GLOB sources *.cpp )
file( GLOB_RECURSE headers  *.h )
target_sources( ${CMAKE_PROJECT_NAME} PRIVATE ${sources} ${headers} ${protos} )

target_link_libraries( ${CMAKE_PROJECT_NAME}  GTest::gtest_main )
if( MSVC )
	set_target_properties( ${CMAKE_PROJECT_NAME} PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "-c" )
	target_link_libraries( ${CMAKE_PROJECT_NAME} fmt::fmt )
#	include( GoogleTest )
#	gtest_discover_tests( ${CMAKE_PROJECT_NAME} )
else()
	target_link_libraries( ${CMAKE_PROJECT_NAME} Jde )
	target_link_libraries( ${CMAKE_PROJECT_NAME} Jde.Crypto )
	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		target_link_libraries( ${CMAKE_PROJECT_NAME} stdc++ )
		target_link_libraries( ${CMAKE_PROJECT_NAME} stdc++fs )
	endif()
	target_link_libraries( ${CMAKE_PROJECT_NAME} libcrypto.a )
	target_link_libraries( ${CMAKE_PROJECT_NAME} Boost::json )
	target_link_libraries( ${CMAKE_PROJECT_NAME} jsonnet++ ) #libjsonnet.a  libryml.a
	target_link_libraries( ${CMAKE_PROJECT_NAME} debug libfmtd.a optimized libfmt.a )
#	target_link_libraries( ${CMAKE_PROJECT_NAME} protobuf::libprotobuf libutf8_range.a libutf8_validity.a )
#	target_link_libraries( ${CMAKE_PROJECT_NAME} absl::cord absl::log_internal_check_op absl::log_internal_conditions absl::log_internal_message absl::raw_hash_set absl::status absl::statusor ) #absl::strings
endif()


if( MSVC )
	add_custom_command( TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
		$ENV{JDE_DIR}/Framework/source/.build/.bin/$(ConfigurationName)/Jde.dll
		$ENV{JDE_DIR}/Public/src/crypto/.build/.bin/$(ConfigurationName)/Jde.Crypto.dll
		${CMAKE_CURRENT_SOURCE_DIR}/config/Crypto.Tests.jsonnet
		${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$(ConfigurationName) COMMENT "copy dlls" )

	string( APPEND cmd "$<IF:$<CONFIG:Release>,"
			"${CMAKE_COMMAND};-E;echo;release,"
      "${CMAKE_COMMAND};-E;copy_if_different;$ENV{JDE_DIR}/Framework/source/.build/.bin/$(ConfigurationName)/Jde.pdb;$ENV{JDE_DIR}/Public/src/crypto/.build/.bin/$(ConfigurationName)/Jde.Crypto.pdb"
    ">")
	add_custom_command( TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND "${cmd}" COMMAND_EXPAND_LISTS COMMENT "copy pdbs" )
endif()

target_precompile_headers( ${CMAKE_PROJECT_NAME} PRIVATE
	<openssl/engine.h>
	<openssl/hmac.h>
	<openssl/err.h>
	<openssl/evp.h>
	<openssl/rsa.h>
	<gtest/gtest.h>
	<jde/framework.h>
)