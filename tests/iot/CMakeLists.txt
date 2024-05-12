cmake_minimum_required(VERSION 3.16)
project( Tests.Iot VERSION 1.0.0.0 )
set( CMAKE_MODULE_PATH $ENV{JDE_DIR}/Public/build )
include(common)
include(tests)
link_directories( ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} )
link_directories( ${fmt_DIR}/../.. )
link_directories( ${Protobuf_DIR}/../.. )

add_executable( ${CMAKE_PROJECT_NAME} main.cpp 
	OpenSslTests.cpp DBTests.cpp OpcUserTests.cpp )

target_link_libraries( ${CMAKE_PROJECT_NAME} Jde)
target_link_libraries( ${CMAKE_PROJECT_NAME} Jde.Crypto)
target_link_libraries( ${CMAKE_PROJECT_NAME} gtest_main)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_link_libraries( ${CMAKE_PROJECT_NAME} stdc++)
	target_link_libraries( ${CMAKE_PROJECT_NAME} stdc++fs)
endif()	
target_link_libraries( ${CMAKE_PROJECT_NAME} libcrypto.a)
target_link_libraries( ${CMAKE_PROJECT_NAME} debug libfmtd.a optimized libfmt.a)

target_link_libraries( ${CMAKE_PROJECT_NAME} protobuf::libprotobuf libutf8_range.a libutf8_validity.a )
target_link_libraries( ${CMAKE_PROJECT_NAME} absl::cord absl::log_internal_check_op absl::log_internal_conditions absl::log_internal_message absl::raw_hash_set absl::status absl::statusor ) #absl::strings

target_precompile_headers( ${CMAKE_PROJECT_NAME} PRIVATE
	<openssl/engine.h>
	<openssl/hmac.h>
	<openssl/err.h>
	<openssl/evp.h>
	<openssl/rsa.h>
	<gtest/gtest.h>
	<jde/TypeDefs.h>
	<jde/Exception.h>
	<jde/Log.h>
)