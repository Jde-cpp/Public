cmake_minimum_required(VERSION 3.16)
project(Jde.DB.Sqlite)
set( CMAKE_MODULE_PATH $ENV{JDE_DIR}/Framework/cmake )
include(SetUpConfigurations)
#######################################################
add_library( Jde.DB.Sqlite SHARED SqliteDS.cpp  )
#######################################################
target_precompile_headers(Jde.DB.Sqlite
  PRIVATE
	<boost/crc.hpp>
	<boost/system/error_code.hpp>
	<spdlog/spdlog.h>
	<spdlog/sinks/basic_file_sink.h>
	<spdlog/fmt/ostr.h>
	<jde/Exception.h>
)
