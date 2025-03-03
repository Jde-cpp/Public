cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( ../../../build/functions.cmake )
project( Jde.Web.Tests VERSION ${JDE_VERSION} LANGUAGES CXX )
include( ../../../build/dependencies.cmake )

set( targetName Jde.Web.Tests )
add_executable( ${targetName} )
file( GLOB sources *.cpp **/*.cpp )
file( GLOB headers *.h **/*.h )
file( GLOB protos **/*.proto )

target_sources( ${targetName} PRIVATE ${sources} ${headers} ${protos} )
if( CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR )
	add_subdirectory( $ENV{JDE_DIR}/Framework/source framework )
	add_subdirectory( ../../app/shared app/shared )
	add_subdirectory( ../../crypto/src crypto )
	add_subdirectory( ../../db/src db )
	add_subdirectory( ../../ql ql )
	add_subdirectory( ../client web/client )
	add_subdirectory( ../server web/server )
endif()

set( CMAKE_INCLUDE_CURRENT_DIR ON )

find_package( GTest REQUIRED )
target_link_libraries( ${targetName} GTest::gtest_main )
target_link_libraries( ${targetName} ZLIB::ZLIB )
target_link_libraries( ${targetName} Jde Jde.App.Shared Jde.Crypto Jde.DB Jde.QL Jde.Web.Client Jde.Web.Server )
target_link_libraries( ${targetName} absl::cord absl::log_internal_check_op absl::log_internal_conditions absl::log_internal_message absl::raw_hash_set absl::status absl::statusor ) #absl::strings
target_link_libraries( ${targetName} fmt::fmt )
target_link_libraries( ${targetName} protobuf::libprotobuf ) #libutf8_range.a libutf8_validity.a
target_link_libraries( ${targetName} libssl.a )
target_link_libraries( ${targetName} libcrypto.a )
target_link_libraries( ${targetName} Boost::json )
link_directories( $ENV{LIB_DIR}/jsonnet )
target_link_libraries( ${targetName} ${JSONNET_DIR}/lib/libjsonnet++.so ${JSONNET_DIR}/lib/libjsonnet.so )

include_directories( ${targetName} $ENV{JDE_DIR}/Public/include/jde/web/client/proto )
include_directories( ${targetName} $ENV{JDE_DIR}/Public/include/app/shared/proto )

target_precompile_headers( ${targetName} PRIVATE
	pc.h
)

protobuf_generate(
	TARGET ${targetName}
	PROTOC_OUT_DIR ${PROJECT_SOURCE_DIR}
)
