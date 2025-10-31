cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( $ENV{JDE_BASH}/Public/build/functions.cmake )
project( Jde.Framework.Tests VERSION ${JDE_VERSION} LANGUAGES CXX )
include( $ENV{JDE_BASH}/Public/build/dependencies.cmake )
message( Framework.Tests )
find_package( GTest REQUIRED )

set( targetName Jde.Framework.Tests )
add_executable( ${targetName} )
compileOptions( ${targetName} )
# if( WIN32)
# 	set_target_properties( ${targetName} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>" )
# endif()

file( GLOB sources *.cpp log/*.cpp crypto/*.cpp )
file( GLOB_RECURSE headers *.h log/*.h )
target_sources( ${targetName} PRIVATE ${sources} ${headers} )

target_link_libraries( ${targetName} Boost::json )
target_link_libraries( ${targetName} GTest::gtest GTest::gtest_main )
target_link_libraries( ${targetName} fmt::fmt )
target_link_libraries( ${targetName} Jde )
if( WIN32 )
	include_directories( ${OPENSSL_ROOT_DIR}/include )
	set_target_properties( ${targetName} PROPERTIES LINK_FLAGS ${CMAKE_EXE_LINKER_FLAGS} )
else()
	target_link_libraries( ${targetName} libssl.a libcrypto.a )
endif()

#add_test( NAME FrameworkTests COMMAND $<TARGET_FILE:Jde.Framework.Tests> )
#set( CMAKE_CTEST_ARGUMENTS "-settings=${CMAKE_CURRENT_SOURCE_DIR}/config/Framework.Tests.jsonnet" )


#set( cmake_ctest_arguments -tests -settings=$ENV{jde_dir}/public/libs/fwk/tests/config/Framework.Tests.jsonnet )
add_test( NAME FwkTests COMMAND $<TARGET_FILE:Jde.Framework.Tests> -tests -settings=$ENV{jde_dir}/public/libs/fwk/tests/config/Framework.Tests.jsonnet )
#add_test( name Jde.Framework.Tests )

#add_dependencies( ${targetName} Jde )

target_precompile_headers( ${targetName} PRIVATE pc.h )

if( WIN32 )
	set( buildLibDir ${CMAKE_BINARY_DIR}/../../libs )
	cmake_path( ABSOLUTE_PATH buildLibDir NORMALIZE OUTPUT_VARIABLE buildLibDir )
	set_target_properties( ${targetName} PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "-settings=${CMAKE_CURRENT_SOURCE_DIR}/config/Framework.Tests.jsonnet" )
	copyLibDlls()
endif()