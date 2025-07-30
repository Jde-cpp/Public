cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
enable_testing()
include( ../../../build/functions.cmake )
project( Jde.Web.Tests VERSION ${JDE_VERSION} LANGUAGES CXX )
include( ../../../build/dependencies.cmake )

if( WIN32 )
	add_definitions( -DZLIB_USE_STATIC_LIBS -D )
	set( ZLIB_ROOT ${LIB_ROOT}/zlib )
endif()
find_package( ZLIB REQUIRED )


set( targetName Jde.Web.Tests )
add_executable( ${targetName} )
compileOptions( ${targetName} )
if( WIN32)
	set_target_properties( ${targetName} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>" )
endif()

file( GLOB sources *.cpp **/*.cpp )
file( GLOB headers *.h **/*.h )
file( GLOB protos **/*.proto )
target_sources( ${targetName} PRIVATE ${sources} ${headers} ${protos} )

add_subdirectory( $ENV{JDE_DIR}/Framework/source ../../framework )
add_subdirectory( ../../access/src ../../access/src )
add_subdirectory( ../../app/shared ../../app/shared )
add_subdirectory( ../../crypto/src ../../crypto/lib )
add_subdirectory( ../../db/src ../../db/lib )
add_subdirectory( ../../ql ../../ql )

set( CMAKE_INCLUDE_CURRENT_DIR ON )

target_link_libraries( ${targetName} Boost::json )
find_package( GTest REQUIRED )
target_link_libraries( ${targetName} GTest::gtest_main )
target_link_libraries( ${targetName} ZLIB::ZLIB )
target_link_libraries( ${targetName} Jde.App.Shared Jde.Web.Server )
target_link_libraries( ${targetName} absl::cord absl::log_internal_check_op absl::log_internal_conditions absl::log_internal_message absl::raw_hash_set absl::status absl::statusor ) #absl::strings
target_link_libraries( ${targetName} fmt::fmt )
target_link_libraries( ${targetName} protobuf::libprotobuf ) #libutf8_range.a libutf8_validity.a
if( WIN32 )
	include_directories( ${targetName} ${ZLIB_INCLUDE_DIRS} )
else()
	target_link_libraries( ${targetName} libssl.a )
	target_link_libraries( ${targetName} libcrypto.a )
	set_target_properties( ${targetName} PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "-tests -settings=${CMAKE_CURRENT_SOURCE_DIR}/config/Web.Tests.jsonnet" )
endif()

include_directories( ${targetName} $ENV{JDE_DIR}/Public/include/jde/web/client/proto )
include_directories( ${targetName} $ENV{JDE_DIR}/Public/include/app/shared/proto )

add_test( NAME WebTests COMMAND $<TARGET_FILE:Jde.Web.Tests> )
set( CMAKE_CTEST_ARGUMENTS "-settings=${CMAKE_CURRENT_SOURCE_DIR}/config/Web.Tests.jsonnet" )
if( WIN32 )
	set_target_properties( ${targetName} PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "-tests -settings=${CMAKE_CURRENT_SOURCE_DIR}/config/Web.Tests.jsonnet" )
	copyCommonDlls()
endif()

target_precompile_headers( ${targetName} PRIVATE
	pc.h
)

protobuf_generate(
	TARGET ${targetName}
	PROTOC_OUT_DIR ${PROJECT_SOURCE_DIR}
)