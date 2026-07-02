cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( $ENV{JDE_DIR}/build/functions.cmake )
project( Framework VERSION ${JDE_VERSION} LANGUAGES CXX )
message( "${PROJECT_NAME}:  ${PROJECT_VERSION}" )
include( dependencies.cmake )

if( WIN32 )
	find_package( OpenSSL REQUIRED )
	find_package( ryml REQUIRED )
endif()

set( targetName Jde )
add_library( ${targetName} SHARED )
compileOptions( ${targetName} )
set_property( TARGET ${targetName} PROPERTY POSITION_INDEPENDENT_CODE ON )
if( WIN32 )
	set_target_properties( ${targetName} PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
		ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
		PDB_OUTPUT_DIRECTORY     "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
	)
endif()
target_compile_definitions( ${targetName} PUBLIC SPDLOG_FMT_EXTERNAL )
#if( WIN32 )
#	target_link_directories( ${targetName} PRIVATE $ENV{REPO_DIR}/jsonnet/vs2017/x64/${CMAKE_BUILD_TYPE} )
#endif()

file(GLOB sources *.cpp **/*.cpp **/**/*.cpp )
file(GLOB headers "**/*.h")

set( osDir ${CMAKE_CURRENT_SOURCE_DIR}/process/os )
if( WIN32 )
	set( osDir ${osDir}/windows )
	list( APPEND sources ${osDir}/WindowsApp.cpp ${osDir}/WindowsDrive.cpp ${osDir}/WindowsSvc.cpp  ${osDir}/WindowsThread.cpp ${osDir}/WindowsUtilities.cpp  ${osDir}/WindowsWorker.cpp ${osDir}/WinFileAwait.cpp )
else()
	set( osDir ${osDir}/linux )
  list( APPEND sources ${osDir}/LinuxApp.cpp ${osDir}/LinuxDrive.cpp ${osDir}/LinuxFileAwait.cpp ${osDir}/LinuxThread.cpp )
	find_package( PkgConfig REQUIRED )
	pkg_check_modules( uring REQUIRED IMPORTED_TARGET liburing )
endif()
get_filename_component(thread_cpp ${CMAKE_CURRENT_SOURCE_DIR}/process/thread.cpp ABSOLUTE)
list(REMOVE_ITEM sources "${thread_cpp}")

target_sources( ${targetName} PRIVATE ${sources} ${headers} )

target_link_libraries( ${targetName} PUBLIC fmt::fmt )
#if( Boost_USE_STATIC_LIBS )
#	message( "Using static Boost libraries" )
	target_link_libraries( ${targetName} PUBLIC Boost::json )
#endif()

if( WIN32 )
	target_link_libraries( ${targetName} PRIVATE version )
	target_link_libraries( ${targetName} PRIVATE jsonnet jsonnet++ ryml::ryml )
else()
	target_link_libraries( ${targetName} PRIVATE libjsonnet++.so )
	target_link_libraries( ${targetName} PRIVATE PkgConfig::uring )
#	target_link_libraries( ${targetName} PRIVATE libssl.a libcrypto.a )
endif()
target_link_libraries( ${targetName} PUBLIC OpenSSL::SSL )

target_precompile_headers( ${targetName}
  PUBLIC
  <jde/fwk.h>
)
