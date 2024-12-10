cmake_minimum_required(VERSION 3.20)
set( CMAKE_MODULE_PATH $ENV{JDE_DIR}/Public/build )
include(common)
findBoost()
if( ${VCPKG} )
	SET( CMAKE_TOOLCHAIN_FILE $ENV{REPO_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake  CACHE STRING "Vcpkg toolchain file" )
endif()

project( Jde.Web.Tests VERSION 1.0.0.0 )
include( compiler )
if( NOT ${VCPKG} )
	include( tests )
endif()

#find_package( Protobuf REQUIRED ) in compiler.cmake
#include_directories( ${CMAKE_CURRENT_BINARY_DIR} )
if( MSVC )
	find_package( OpenSsl REQUIRED )
  link_directories( $ENV{JDE_DIR}/Framework/source/.build/.bin/${outDir} )
  link_directories( ../../src/web/client/.build/.bin/${outDir} )
  link_directories( ../../src/web/server/.build/.bin/${outDir} )
  link_directories( ../../src/crypto/.build/.bin/${outDir} )
  link_directories( ../../src/app/shared/.build/.bin/${outDir} )
else()
	link_directories( ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} )
	link_directories( ${fmt_DIR}/../.. )
	link_directories( ${Protobuf_DIR}/../.. )
	link_directories( $ENV{REPO_DIR}/install/g++-13/asan/jsonnet/lib/ )
endif()

add_executable( ${CMAKE_PROJECT_NAME} )
file( GLOB sources *.cpp **/*.cpp )
file( GLOB headers *.h **/*.h )
file( GLOB protos "**/*.proto" )
set( CMAKE_INCLUDE_CURRENT_DIR ON )

target_sources( ${CMAKE_PROJECT_NAME} PRIVATE ${sources} ${headers} ${protos} )

find_package( GTest REQUIRED )
target_link_libraries( ${CMAKE_PROJECT_NAME}  GTest::gtest_main )
if( MSVC )
	set_source_files_properties( test.pb.cc PROPERTIES COMPILE_FLAGS "/W4" )
	set_target_properties( ${CMAKE_PROJECT_NAME} PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "-c" )
	target_link_libraries( ${CMAKE_PROJECT_NAME} OpenSSL::SSL )
	find_package( fmt )
	target_link_libraries( ${CMAKE_PROJECT_NAME} fmt::fmt )
	target_link_libraries( ${CMAKE_PROJECT_NAME} protobuf::libprotobuf )
	include_directories( ${OPENSSL_INCLUDE_DIR} )
else()
	target_link_libraries( ${CMAKE_PROJECT_NAME} ZLIB::ZLIB )
	target_link_libraries( ${CMAKE_PROJECT_NAME} Jde Jde.App.Shared Jde.Crypto Jde.DB Jde.QL Jde.Web.Client Jde.Web.Server )
	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		target_link_libraries( ${CMAKE_PROJECT_NAME} stdc++ )
		target_link_libraries( ${CMAKE_PROJECT_NAME} stdc++fs )
	endif()
	target_link_libraries( ${CMAKE_PROJECT_NAME} absl::cord absl::log_internal_check_op absl::log_internal_conditions absl::log_internal_message absl::raw_hash_set absl::status absl::statusor ) #absl::strings
	target_link_libraries( ${CMAKE_PROJECT_NAME} debug libfmtd.a optimized libfmt.a )
	target_link_libraries( ${CMAKE_PROJECT_NAME} protobuf::libprotobuf libutf8_range.a libutf8_validity.a )
	target_link_libraries( ${CMAKE_PROJECT_NAME} lzma )
	target_link_libraries( ${CMAKE_PROJECT_NAME} libssl.a )
	target_link_libraries( ${CMAKE_PROJECT_NAME} libcrypto.a )
	target_link_libraries( ${CMAKE_PROJECT_NAME} Boost::json )
	target_link_libraries( ${CMAKE_PROJECT_NAME} jsonnet++ ) #libjsonnet.a  libryml.a
endif()
target_precompile_headers( ${CMAKE_PROJECT_NAME} PRIVATE
	pc.h
)

set( protoExeImportDir ${CMAKE_CURRENT_LIST_DIR}/.. )
file( REAL_PATH ../../src/http httpDir BASE_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}" )

set( importProtoConfigDir ${httpDir}/proto )
include_directories( ${httpDir}/.obj/${outDir} )

if( NOT EXISTS ${protoExeImportDir}/FromServer.proto )
	execute_process( COMMAND ${CMAKE_COMMAND} -E create_symlink ${importProtoConfigDir}/FromServer.proto ${protoExeImportDir}/FromServer.proto )
endif()
if( NOT EXISTS ${protoExeImportDir}/FromClient.proto )
	execute_process( COMMAND ${CMAKE_COMMAND} -E create_symlink ${importProtoConfigDir}/FromClient.proto ${protoExeImportDir}/FromClient.proto )
endif()
protobuf_generate(
	TARGET ${CMAKE_PROJECT_NAME}
	IMPORT_DIRS ${protoExeImportDir} )

if( MSVC )
	set( externals ${CMAKE_CURRENT_BINARY_DIR}/externals/$(ConfigurationName) )
	set( subdir .build/.bin/$(ConfigurationName) )
	set( bin ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$(ConfigurationName) )

	add_custom_command( TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $ENV{JDE_DIR}/Framework/source/${subdir}/Jde.dll ${bin}/Jde.dll )
	add_custom_command( TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $ENV{JDE_DIR}/Framework/source/${subdir}/Jde.pdb ${bin}/Jde.pdb )
	add_custom_command( TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $ENV{JDE_DIR}/Public/src/crypto/${subdir}/Jde.Crypto.dll ${bin}/Jde.Crypto.dll )
	add_custom_command( TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $ENV{JDE_DIR}/Public/src/crypto/${subdir}/Jde.Crypto.pdb ${bin}/Jde.Crypto.pdb )
	add_custom_command( TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $ENV{JDE_DIR}/Public/src/web/client/${subdir}/Jde.Web.Client.dll ${bin}/Jde.Web.Client.dll )
	add_custom_command( TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $ENV{JDE_DIR}/Public/src/web/client/${subdir}/Jde.Web.Client.pdb ${bin}/Jde.Web.Client.pdb )
	add_custom_command( TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $ENV{JDE_DIR}/Public/src/web/server/${subdir}/Jde.Web.Server.dll ${bin}/Jde.Web.Server.dll )
	add_custom_command( TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $ENV{JDE_DIR}/Public/src/web/server/${subdir}/Jde.Web.Server.pdb ${bin}/Jde.Web.Server.pdb )
	add_custom_command( TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $ENV{JDE_DIR}/Public/src/app/shared/${subdir}/Jde.App.Shared.dll ${bin}/Jde.App.Shared.dll )
	add_custom_command( TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink $ENV{JDE_DIR}/Public/src/app/shared/${subdir}/Jde.App.Shared.pdb ${bin}/Jde.App.Shared.pdb )

	#add_custom_command( TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E make_directory  ${externals} )
	# add_custom_command( TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
	# 	$ENV{JDE_DIR}/Framework/source/${subdir}/Jde.dll
	# 	$ENV{JDE_DIR}/Framework/source/${subdir}/Jde.pdb
	# 	$ENV{JDE_DIR}/Public/src/crypto/${subdir}/Jde.Crypto.dll
	# 	$ENV{JDE_DIR}/Public/src/crypto/${subdir}/Jde.Crypto.pdb
	# 	$ENV{JDE_DIR}/Public/src/web/client/${subdir}/Jde.Web.Client.dll
	# 	$ENV{JDE_DIR}/Public/src/web/client/${subdir}/Jde.Web.Client.pdb
	# 	$ENV{JDE_DIR}/Public/src/web/server/${subdir}/Jde.Web.Server.dll
	# 	$ENV{JDE_DIR}/Public/src/web/server/${subdir}/Jde.Web.Server.pdb
	# 	$ENV{JDE_DIR}/Public/src/app/shared/${subdir}/Jde.App.Shared.dll
	# 	$ENV{JDE_DIR}/Public/src/app/shared/${subdir}/Jde.App.Shared.pdb
	# 	${externals} COMMENT "copy dlls" )

		# add_custom_command( TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${externals}/Jde.dll ${bin}/Jde.dll )
		# add_custom_command( TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${externals}/Jde.pdb ${bin}/Jde.pdb )
		# add_custom_command( TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${externals}/Jde.Crypto.dll ${bin}/Jde.Crypto.dll )
		# add_custom_command( TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${externals}/Jde.Crypto.pdb ${bin}/Jde.Crypto.pdb )
		# add_custom_command( TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${externals}/Jde.Web.Client.dll ${bin}/Jde.Web.Client.dll )
		# add_custom_command( TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${externals}/Jde.Web.Client.pdb ${bin}/Jde.Web.Client.pdb )
		# add_custom_command( TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${externals}/Jde.Web.Server.dll ${bin}/Jde.Web.Server.dll )
		# add_custom_command( TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${externals}/Jde.Web.Server.pdb ${bin}/Jde.Web.Server.pdb )
		# add_custom_command( TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${externals}/Jde.App.Shared.dll ${bin}/Jde.App.Shared.dll )
		# add_custom_command( TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E create_symlink ${externals}/Jde.App.Shared.pdb ${bin}/Jde.App.Shared.pdb )
endif()
