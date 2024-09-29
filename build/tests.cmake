#https://stackoverflow.com/questions/31546278/where-to-set-cmake-configuration-types-in-a-project-with-subprojects
cmake_minimum_required(VERSION 3.20)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
#dump_cmake_variables()
if(MSVC)
	include_directories( ${googletest_SOURCE_DIR}/googletest/include )
endif()