cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
enable_testing()
include( ../../../build/functions.cmake )
project( Jde.Web.Tests VERSION ${JDE_VERSION} LANGUAGES CXX )
include( ../../../build/dependencies.cmake )

if( WIN32 )
	add_definitions( -DZLIB_USE_STATIC_LIBS -D )
	set( ZLIB_ROOT ${LIB_ROOT}/zlib )
	if( ${OPENSSL_ROOT_DIR} STREQUAL "" )
		message( FATAL_ERROR "Set OPENSSL_ROOT_DIR cmake variable" )
	endif()
	set( OPENSSL_USE_STATIC_LIBS ON )
	find_package( OpenSSL REQUIRED )
endif()
find_package( ZLIB REQUIRED )
if( WIN32)
	set_target_properties( ${targetName} PROPERTIES LINK_FLAGS ${CMAKE_EXE_LINKER_FLAGS} )
elseif( NOT CMAKE_BUILD_TYPE STREQUAL "Debug" )
	set( jsonnet_DIR ${LIB_DIR}/jsonnet CACHE PATH "Jsonnet directory" )
	link_directories( ${jsonnet_DIR}/lib )
endif()

set( targetName Jde.Web.Tests )
add_executable( ${targetName} )
compileOptions( ${targetName} )


file( GLOB sources *.cpp **/*.cpp )
file( GLOB headers *.h **/*.h )
file( GLOB protos **/*.proto )
target_sources( ${targetName} PRIVATE ${sources} ${headers} ${protos} )

if( NOT jde_Superbuild )
	add_subdirectory( ../../fwk/src ../../fwk/lib )
	add_subdirectory( ../../access/src ../../access/lib )
	add_subdirectory( ../../app/shared ../../app/shared )
	add_subdirectory( ../../db/src ../../db/lib )
	add_subdirectory( ../../ql ../../ql )
endif()

set( CMAKE_INCLUDE_CURRENT_DIR ON )

target_link_libraries( ${targetName} Boost::json )
find_package( GTest REQUIRED )
target_link_libraries( ${targetName} GTest::gtest_main )
#target_link_libraries( ${targetName} ZLIB::ZLIB )
target_link_libraries( ${targetName} Jde.App.Shared Jde.Web.Server )
target_link_libraries( ${targetName} absl::cord absl::log_internal_check_op absl::log_internal_conditions absl::log_internal_message absl::raw_hash_set absl::status absl::statusor ) #absl::strings
target_link_libraries( ${targetName} fmt::fmt )
target_link_libraries( ${targetName} protobuf::libprotobuf ) #libutf8_range.a libutf8_validity.a
if( WIN32 )
	include_directories( ${targetName} ${ZLIB_INCLUDE_DIRS} )
	target_link_libraries( ${targetName} OpenSSL::SSL )
else()
	target_link_libraries( ${targetName} libssl.a )
	target_link_libraries( ${targetName} libcrypto.a )
	set_target_properties( ${targetName} PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "-tests -settings=${CMAKE_CURRENT_SOURCE_DIR}/config/Web.Tests.jsonnet" )
endif()

include_directories( ${targetName} $ENV{JDE_DIR}/Public/include/jde/web/client/proto )
include_directories( ${targetName} $ENV{JDE_DIR}/Public/include/app/shared/proto )

add_test( NAME WebTests COMMAND $<TARGET_FILE:Jde.Web.Tests> -tests -settings=${CMAKE_CURRENT_SOURCE_DIR}/config/Web.Tests.jsonnet )
set( CMAKE_CTEST_ARGUMENTS "-settings=${CMAKE_CURRENT_SOURCE_DIR}/config/Web.Tests.jsonnet" )
if( WIN32 )
	set_target_properties( ${targetName} PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "-tests -settings=${CMAKE_CURRENT_SOURCE_DIR}/config/Web.Tests.jsonnet" )
	copyCommonDlls()
  set_property( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${targetName} )
else()
	if( NOT CMAKE_BUILD_TYPE STREQUAL "Debug" )
		target_link_libraries( ${targetName} libjsonnet++.a libjsonnet.a ) #jsonnet
		linkRepo( ${CMAKE_BINARY_DIR}/.. ${CMAKE_CURRENT_LIST_DIR} )
	endif()
endif()

target_precompile_headers( ${targetName} PRIVATE
	pc.h
)

protobuf_generate(
	TARGET ${targetName}
	PROTOC_OUT_DIR ${PROJECT_SOURCE_DIR}
)