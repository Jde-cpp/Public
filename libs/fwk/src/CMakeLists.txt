cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( $ENV{JDE_DIR}/Public/build/functions.cmake )
project( Framework VERSION ${JDE_VERSION} LANGUAGES CXX )
message( "${PROJECT_NAME}:  ${PROJECT_VERSION}" )
include( dependencies.cmake )

set( targetName Jde )
add_library( ${targetName} SHARED )
compileOptions( ${targetName} )
set_property( TARGET ${targetName} PROPERTY POSITION_INDEPENDENT_CODE ON )

file(GLOB sources *.cpp **/*.cpp **/**/*.cpp )
file(GLOB headers "**/*.h")

set( newDir $ENV{JDE_DIR}/Public/libs/fwk/src )
set( osDir ${newDir}/process/os )
if( WIN32 )
	set( CMAKE_SHARED_LINKER_FLAGS_DEBUG "/debug /INCREMENTAL:NO" )
	set( osDir ${osDir}/windows )
	list( APPEND sources ${osDir}/WindowsApp.cpp ${osDir}/WindowsDrive.cpp ${osDir}/WindowsSvc.cpp  ${osDir}/WindowsThread.cpp ${osDir}/WindowsUtilities.cpp  ${osDir}/WindowsWorker.cpp ${osDir}/WinFileAwait.cpp )
else()
  list( APPEND sources ${osDir}/linux/LinuxApp.cpp ${osDir}/linux/LinuxDrive.cpp ${osDir}/linux/LinuxFileAwait.cpp ${osDir}/linux/LinuxThread.cpp )
	find_package( PkgConfig REQUIRED )
	pkg_check_modules( uring REQUIRED IMPORTED_TARGET liburing )
endif()
get_filename_component(thread_cpp ${CMAKE_CURRENT_SOURCE_DIR}/process/thread.cpp ABSOLUTE)
list(REMOVE_ITEM sources "${thread_cpp}")

target_sources( ${targetName} PRIVATE ${sources} ${headers} )

target_link_libraries( ${targetName} PUBLIC fmt::fmt )
if( WIN32 )
	target_link_libraries( ${targetName} PRIVATE version )
	target_link_libraries( ${targetName} PUBLIC Boost::json )
	target_link_libraries( ${targetName} PRIVATE jsonnet )
else()
	target_link_libraries( ${targetName} PRIVATE jsonnet++ jsonnet )
	target_link_libraries( ${targetName} PRIVATE PkgConfig::uring )
endif()

target_precompile_headers( ${targetName}
  PUBLIC
  <jde/fwk.h>
)
