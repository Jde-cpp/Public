cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( ../../../../build/functions.cmake )
project( Jde.DB.MySql VERSION ${JDE_VERSION} LANGUAGES CXX )
message( ${PROJECT_NAME} )
include( ../../../../build/dependencies.cmake )

#project(boost_mysql_example LANGUAGES CXX)

find_package( Boost REQUIRED COMPONENTS charconv )
find_package( Threads REQUIRED )

set( targetName Jde.DB.MySql )
add_library( ${targetName} SHARED )

set_property( TARGET ${targetName} PROPERTY POSITION_INDEPENDENT_CODE ON )
file( GLOB sources *.cpp )
file( GLOB headers *.h )

# main Contains Boost.MySQL sources via #include <boost/mysql/src.hpp>
target_sources(${targetName} PRIVATE ${sources} ${headers} )
target_link_directories( ${targetName} PRIVATE "../../../")
target_link_libraries(${targetName} PRIVATE Boost::charconv Threads::Threads OpenSSL::Crypto OpenSSL::SSL)

# We need to define BOOST_MYSQL_SEPARATE_COMPILATION in any code using Boost.MySQL in separate-build mode
target_compile_definitions(${targetName} PRIVATE BOOST_MYSQL_SEPARATE_COMPILATION)

set_property( TARGET ${targetName} PROPERTY POSITION_INDEPENDENT_CODE ON )
file(GLOB sources *.cpp )
file(GLOB headers *.h )
target_sources(${targetName} PRIVATE ${sources} ${headers} )

target_link_libraries( ${targetName} PRIVATE fmt::fmt )
target_link_libraries( ${targetName} PRIVATE Jde.DB )

target_precompile_headers( ${targetName}
  PRIVATE
	<boost/mysql.hpp>
	<jde/framework.h>
	<jde/framework/str.h>
	<jde/framework/io/json.h>
	../../../../../Framework/source/DateTime.h
	usings.h
)
