#cls;rm CMakeCache.txt;cmake $JDE_DIR/Public/build -DCMAKE_BUILD_TYPE=Debug|Release|RelWithDebInfo|MinSizeRel
cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( ExternalProject )
include( ${CMAKE_CURRENT_SOURCE_DIR}/functions.cmake )
project( libs VERSION ${JDE_VERSION} LANGUAGES CXX )
if( CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT )
  set( CMAKE_INSTALL_PREFIX $ENV{LIB_DIR} CACHE PATH <comment> FORCE )
endif()

set( stdArgs
	-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
	-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
	-DCMAKE_CXX_FLAGS_DEBUG=${CMAKE_CXX_FLAGS_DEBUG}
	-DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
	-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
	-DCMAKE_POSITION_INDEPENDENT_CODE=ON
	-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
	-DCMAKE_SHARED_LINKER_FLAGS="-Wl,-rpath=$ORIGIN"
)

ExternalProject_Add(gtest
	GIT_REPOSITORY https://github.com/google/googletest.git
	GIT_TAG v1.16.0
	PREFIX gtest
	INSTALL_DIR $ENV{LIB_DIR}/gtest
	CMAKE_ARGS ${stdArgs}
)

ExternalProject_Add(fmt
	GIT_REPOSITORY https://github.com/fmtlib/fmt.git
	GIT_TAG 11.1.4
	PREFIX fmt
#	UPDATE_COMMAND git pull https://github.com/fmtlib/fmt.git
	INSTALL_DIR $ENV{LIB_DIR}/fmt
	CMAKE_ARGS ${stdArgs}
		-DBUILD_SHARED_LIBS=ON
#-DCMAKE_POSITION_INDEPENDENT_CODE
)
ExternalProject_Add(spdlog
	GIT_REPOSITORY https://github.com/gabime/spdlog.git
	GIT_TAG v1.15.1
	PREFIX spdlog
	INSTALL_DIR $ENV{LIB_DIR}/spdlog
	CMAKE_ARGS ${stdArgs}
		-DCMAKE_PREFIX_PATH=$ENV{LIB_DIR}
		-DSPDLOG_BUILD_EXAMPLES:BOOL=OFF
		-DSPDLOG_BUILD_BENCH:BOOL=OFF
		-DSPDLOG_BUILD_TESTS:BOOL=OFF
		-DSPDLOG_FMT_EXTERNAL=ON
		-DSPDLOG_NO_THREAD_ID=ON
)

ExternalProject_Add(jsonnet
	GIT_REPOSITORY https://github.com/google/jsonnet.git
	GIT_TAG v0.20.0
	PREFIX jsonnet
	INSTALL_DIR $ENV{LIB_DIR}/jsonnet
	CMAKE_ARGS ${stdArgs}
		-DCMAKE_PREFIX_PATH=$ENV{LIB_DIR}
		-DUSE_SYSTEM_GTEST=ON
		-DBUILD_TESTS=OFF
)

ExternalProject_Add(absl
	GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
	GIT_TAG 20250127.0
	PREFIX absl
	INSTALL_DIR $ENV{LIB_DIR}/absl
	CMAKE_ARGS ${stdArgs}
		-DCMAKE_PREFIX_PATH=$ENV{LIB_DIR}
		-DABSL_BUILD_TESTING=OFF
		-DABSL_USE_EXTERNAL_GOOGLETEST=ON
		#-DABSL_FIND_GOOGLETEST=ON
		-DABSL_PROPAGATE_CXX_STD=ON
)

ExternalProject_Add(protobuf
	GIT_REPOSITORY https://github.com/protocolbuffers/protobuf.git
	GIT_TAG v25.6
	PREFIX protobuf
	INSTALL_DIR $ENV{LIB_DIR}/protobuf
	CMAKE_ARGS ${stdArgs}
		-DCMAKE_PREFIX_PATH=$ENV{LIB_DIR}
		-Dprotobuf_USE_EXTERNAL_GTEST=ON
		-Dprotobuf_BUILD_TESTS=OFF
		-Dprotobuf_ABSL_PROVIDER=package
)
#need to add set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_GLIBCXX_DEBUG" ) to mysql-concpp/src/mysql-connector-cpp/CMakeLists.txt
ExternalProject_Add(mysql-connector-cpp
	GIT_REPOSITORY https://github.com/mysql/mysql-connector-cpp.git
	GIT_TAG 9.2.0
	PREFIX mysql-concpp
	INSTALL_DIR $ENV{LIB_DIR}/mysql-concpp
	CMAKE_ARGS ${stdArgs}
)

ExternalProject_Add(open62541
	GIT_REPOSITORY https://github.com/Jde-cpp/open62541.git
	PREFIX open62541
	INSTALL_DIR $ENV{LIB_DIR}/open62541
	CMAKE_ARGS ${stdArgs}
		-DUA_ENABLE_ENCRYPTION=ON
		-DUA_ENABLE_ENCRYPTION_OPENSSL=ON
)

ExternalProject_Add_StepDependencies( spdlog configure fmt )
ExternalProject_Add_StepDependencies( jsonnet configure gtest )
ExternalProject_Add_StepDependencies( absl configure gtest )
ExternalProject_Add_StepDependencies( protobuf configure absl )
ExternalProject_Add_StepDependencies( protobuf configure gtest )