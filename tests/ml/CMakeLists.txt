cmake_minimum_required(VERSION 3.16)
project( Tests.ML VERSION 1.0.0.0 )
set( CMAKE_MODULE_PATH $ENV{JDE_DIR}/Framework/cmake )
include(SetUpConfigurations)

set( CMAKE_LIBRARY_OUTPUT_DIRECTORY $ENV{JDE_DIR}/bin/${outDir} )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY $ENV{JDE_DIR}/bin/${outDir} )

configure_file(CMakeLists.txt.in googletest-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

# Prevent overriding the parent project's compiler/linker settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Add googletest directly to our build. This defines the gtest and gtest_main targets.
add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
                 ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
                 EXCLUDE_FROM_ALL)

link_directories( ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} )
add_executable(Tests.ML main.cpp ClassificationTests.cpp )
#include_directories( "${PROJECT_SOURCE_DIR}/../../../../Framework/source" )

target_link_libraries(Tests.ML Jde)
target_link_libraries(Tests.ML gtest_main)
target_link_libraries(Tests.ML Threads::Threads)
target_link_libraries(Tests.ML stdc++)
target_link_libraries(Tests.ML protobuf)

target_precompile_headers(Tests.ML PRIVATE
	#<boost/uuid/uuid.hpp>
	#<boost/uuid/uuid_generators.hpp>
	#<boost/uuid/uuid_io.hpp>
	#<boost/crc.hpp>
	#<boost/system/error_code.hpp>
	#<spdlog/spdlog.h>
	#<spdlog/sinks/basic_file_sink.h>
	#<spdlog/fmt/ostr.h>
	#<nlohmann/json.hpp>
	<jde/Exception.h>
	#<$ENV{JDE_DIR}/Framework/source/Settings.h>
	#<$ENV{JDE_DIR}/Framework/source/Cache.h>
)