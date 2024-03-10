#https://stackoverflow.com/questions/31546278/where-to-set-cmake-configuration-types-in-a-project-with-subprojects
cmake_minimum_required(VERSION 3.20.0)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#set(CMAKE_MESSAGE_CONTEXT_SHOW TRUE)
#set(CMAKE_MESSAGE_LOG_LEVEL STATUS)
#set(Protobuf_USE_STATIC_LIBS OFF)
set_property(GLOBAL PROPERTY DEBUG_CONFIGURATIONS Asan Debug)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
#set(Protobuf_DEBUG ON)
if(NOT SET_UP_CONFIGURATIONS_DONE)
	set(SET_UP_CONFIGURATIONS_DONE 1)
	if(CMAKE_CONFIGURATION_TYPES) # multiconfig generator?
		if(MSVC)
			set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo" CACHE STRING "" FORCE)
		else()
			set(CMAKE_CONFIGURATION_TYPES "Debug;ASAN;Release;RelWithDebInfo" CACHE STRING "" FORCE)
		endif()
	else()
		if(NOT CMAKE_BUILD_TYPE)
			if(MSVC)
				message( "Defaulting to Debug build." )
				set( CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE )
			else()
				message( "Defaulting to ASAN build." )
				set( CMAKE_BUILD_TYPE ASAN CACHE STRING "" FORCE )
			endif()
		endif()
		set_property( CACHE CMAKE_BUILD_TYPE PROPERTY HELPSTRING "Choose the type of build" )
		set_property( CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Asan;Debug;Release;Profile" )
	endif()
endif()
#######################################################
if(MSVC)
	set( Warnings "/W4 /wd\"4068\" /wd\"4251\" /wd\"4275\" /wd\"4297\"" )
else()
	set( Warnings "-Wall -Wno-range-loop-construct -Wno-unknown-pragmas -Wno-empty-body -Wno-exceptions -Wno-subobject-linkage" )
endif()
if( CMAKE_CXX_COMPILER_ID STREQUAL "Clang" )
	set( Warnings "${Warnings} -Wno-unqualified-std-cast-call -Wno-return-type-c-linkage -Wno-#pragma-messages -Wno-deprecated-builtins -Wno-c++11-narrowing" )
endif()
string(APPEND RELEASE " -O3 -march=native " )
string(APPEND RELEASE ${Warnings} )
#add_definitions( -DSPDLOG_USE_STD_FORMAT )
if(MSVC)
	SET(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION 10.0)
	add_definitions( -DBOOST_ALL_DYN_LINK )
	add_definitions( -DWIN32_LEAN_AND_MEAN )
	add_definitions( -D_SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING )
	add_definitions( -D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING )
	
	set(CMAKE_XCODE_SCHEME_ADDRESS_SANITIZER ON)
	set(CMAKE_XCODE_SCHEME_ADDRESS_SANITIZER_USE_AFTER_RETURN ON)	
	string(APPEND CMAKE_CXX_FLAGS_DEBUG " /FC /RTC1 ")
else()
	string(APPEND DEBUG " -O0 -g -D_GLIBCXX_DEBUG ")
endif()
string(APPEND DEBUG ${Warnings} )

string(APPEND CMAKE_CXX_FLAGS_RELEASE ${RELEASE} )
string(APPEND CMAKE_CXX_FLAGS_RELWITHDEBINFO ${RELEASE} )
string(APPEND CMAKE_CXX_FLAGS_DEBUG ${DEBUG})
#string(APPEND CMAKE_CXX_FLAGS_ASAN )
string(APPEND CMAKE_CXX_FLAGS_ASAN "${DEBUG} -fsanitize=address -fno-omit-frame-pointer")
if( CMAKE_CXX_COMPILER_ID STREQUAL "Clang" )
	string(APPEND CMAKE_CXX_FLAGS_ASAN "${DEBUG} -fno-limit-debug-info")
endif()

#add_compile_options("-std=c++23")
set(CMAKE_CXX_STANDARD 23)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
if(MSVC)
	set( outDir "" )
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:preprocessor /Zc:__cplusplus /sdl- /GF" )
else()
	if( NOT CMAKE_BUILD_TYPE MATCHES RelWithDebInfo )
		string(TOLOWER ${CMAKE_BUILD_TYPE} outDir)
	else()
		set( outDir ${CMAKE_BUILD_TYPE} )
	endif()
	if( CMAKE_CXX_COMPILER_ID STREQUAL "Clang" )
		set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++" )
	endif()
endif()

set( cxxPath $ENV{CXX})
cmake_path(GET cxxPath FILENAME compilerSubDir)
set( cmakeLibraryPath $ENV{REPO_DIR}/install/${compilerSubDir}/${outDir} )
list(PREPEND CMAKE_PREFIX_PATH ${cmakeLibraryPath} )

find_package(fmt REQUIRED)
include_directories(${fmt_DIR}/../../../include)
link_directories( ${fmt_DIR}/../.. )
find_package(spdlog REQUIRED)
include_directories(${spdlog_DIR}/../../../include)
find_package(GTest CONFIG REQUIRED)
#if(absl_FOUND)
#	include_directories(${absl_INCLUDE_DIRS})
	#include_directories(/home/duffyj/code/libraries/install/RelWithDebInfo/absl/include)
#endif()	
list(PREPEND CMAKE_PREFIX_PATH ${cmakeLibraryPath}/protobuf/lib/cmake/utf8_range)
find_package(Protobuf CONFIG REQUIRED)
include_directories(${Protobuf_DIR}/../../../include)
include_directories(${absl_DIR}/../../../include)
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.78.0 COMPONENTS)
if(Boost_FOUND)
	include_directories(${Boost_INCLUDE_DIRS})
	#message( "Boost_INCLUDE_DIRS = ${Boost_INCLUDE_DIRS}" )
	#message( "Boost_LIBRARY_DIR = ${Boost_LIBRARY_DIRS}" )
	#message( "BOOST_LIBRARYDIR = ${BOOST_LIBRARYDIR}" )
	#if(MSVC)
		link_directories( ${Boost_LIBRARY_DIRS} )
	#endif()
endif()
#######################################################

#include_directories( "$ENV{REPO_DIR}/spdlog/include" )
include_directories( "$ENV{JDE_DIR}/Public" )
#include_directories( "$ENV{REPO_DIR}/protobuf/src" )

if(MSVC)
	include_directories( $ENV{INCLUDE} )
	include_directories( $ENV{REPO_DIR}/vcpkg/installed/x64-windows/include )
else()
	include_directories( "$ENV{REPO_DIR}/json/include" )
	#include_directories( "$ENV{REPO_DIR}/protobuf/third_party/abseil-cpp" )
	#if( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" )
	#	include_directories( "$ENV{REPO_DIR}/fmt/include" )
	#endif()
endif()
if(MSVC)
	#does not work - appends `pwd` to the path
	set( CMAKE_RUNTIME_OUTPUT_DIRECTORY $(SolutionDir).bin/$(Configuration) )
	set( CMAKE_LIBRARY_OUTPUT_DIRECTORY $(SolutionDir).bin/$(Configuration) )
	set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY $(SolutionDir).bin/$(Configuration) )
	link_directories( $(SolutionDir).bin/$(Configuration) )
else()
	set( CMAKE_LIBRARY_OUTPUT_DIRECTORY $ENV{JDE_DIR}/bin/${outDir} )
	set( CMAKE_RUNTIME_OUTPUT_DIRECTORY $ENV{JDE_DIR}/bin/${outDir} )
endif()	
if(NOT MSVC)
	set(CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS} "-Wl,-rpath=$ORIGIN")
endif()