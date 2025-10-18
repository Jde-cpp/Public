#cls;rm -f CMakeCache.txt;cmake $JDE_DIR/Public/build --preset linux-debug
cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( ExternalProject )
include( ${CMAKE_CURRENT_SOURCE_DIR}/functions.cmake )
project( libs VERSION ${JDE_VERSION} LANGUAGES CXX )
if( CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT )
	message( CMAKE_INSTALL_PREFIX: ${CMAKE_INSTALL_PREFIX} )
  set( CMAKE_INSTALL_PREFIX ${LIB_DIR} CACHE PATH <comment> FORCE )
endif()
message( CMAKE_INSTALL_PREFIX: ${CMAKE_INSTALL_PREFIX} )
message( CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH} )


set( stdArgs
	-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
	-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
	-DCMAKE_CXX_FLAGS_DEBUG=${CMAKE_CXX_FLAGS_DEBUG}
	-DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
	-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
	-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
)
if( WIN32 )
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /MDd /D_DEBUG /TP /fsanitize=address /W1 /ignore:4099")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
else()
	set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-rpath=$ORIGIN" )
	set( stdArgs ${stdArgs} -DCMAKE_POSITION_INDEPENDENT_CODE=ON )
endif()

ExternalProject_Add(gtest
	GIT_REPOSITORY https://github.com/google/googletest.git
	GIT_TAG v1.17.0
	PREFIX gtest
	INSTALL_DIR ${LIB_DIR}/gtest/${CMAKE_BUILD_TYPE}
	LOG_CONFIGURE ON
  LOG_BUILD ON
	CMAKE_C_COMPILER_WORKS ON
	CMAKE_ARGS ${stdArgs}
#		-Dgtest_force_shared_crt=ON
		-DCMAKE_C_COMPILER_WORKS=ON
		-DBUILD_GMOCK=OFF
)

ExternalProject_Add(fmt
	GIT_REPOSITORY https://github.com/fmtlib/fmt.git
	GIT_TAG 12.0.0
	PREFIX fmt
#	UPDATE_COMMAND git pull https://github.com/fmtlib/fmt.git
	INSTALL_DIR ${LIB_DIR}/fmt
	CMAKE_ARGS ${stdArgs}
		-DBUILD_SHARED_LIBS=ON
		-DFMT_MASTER_PROJECT=OFF
		-DFMT_DOC=OFF
		-DFMT_INSTALL=ON
#-DCMAKE_POSITION_INDEPENDENT_CODE
)
ExternalProject_Add(spdlog
	GIT_REPOSITORY https://github.com/gabime/spdlog.git
	GIT_TAG v1.16.0
	PREFIX spdlog
	INSTALL_DIR ${LIB_DIR}/spdlog
	CMAKE_ARGS ${stdArgs}
		-DCMAKE_PREFIX_PATH=${LIB_DIR}
		-DSPDLOG_BUILD_EXAMPLE:BOOL=OFF
		-DSPDLOG_BUILD_BENCH:BOOL=OFF
		-DSPDLOG_BUILD_TESTS:BOOL=OFF
		-DSPDLOG_FMT_EXTERNAL=ON
		-DSPDLOG_NO_THREAD_ID=ON
		-DSPDLOG_BUILD_SHARED=ON
		-DSPDLOG_NO_ATOMIC_LEVELS=ON
		-DSPDLOG_BUILD_PIC=ON
)

ExternalProject_Add(absl
	GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
	GIT_TAG 20250814.1
	PREFIX absl
	INSTALL_DIR ${LIB_DIR}/absl
	CMAKE_ARGS ${stdArgs}
		-DCMAKE_PREFIX_PATH=${LIB_DIR}
		-DABSL_BUILD_TESTING=OFF
		-DABSL_USE_EXTERNAL_GOOGLETEST=ON
		#-DABSL_FIND_GOOGLETEST=ON
		-DABSL_PROPAGATE_CXX_STD=ON
)

ExternalProject_Add(protobuf
	GIT_REPOSITORY https://github.com/protocolbuffers/protobuf.git
	GIT_TAG v32.1
	PREFIX protobuf
	INSTALL_DIR ${LIB_DIR}/protobuf
	CMAKE_ARGS ${stdArgs}
		-DCMAKE_PREFIX_PATH=${LIB_DIR}
		-Dprotobuf_USE_EXTERNAL_GTEST=ON
		-Dprotobuf_BUILD_TESTS=OFF
		-Dprotobuf_ABSL_PROVIDER=package
		-Dprotobuf_LOCAL_DEPENDENCIES_ONLY=ON
		-Dprotobuf_MSVC_STATIC_RUNTIME=OFF
)

IF( WIN32 )
	ExternalProject_Add(zlib
		GIT_REPOSITORY https://github.com/madler/zlib
		GIT_TAG "v1.3.1"
		PREFIX ZLIB
		INSTALL_DIR ${LIB_DIR}/zlib
		CMAKE_ARGS ${stdArgs}
			-DZLIB_BUILD_SHARED=OFF
			-DZLIB_BUILD_TESTING=OFF
	)
	ExternalProject_Add(
			LibXml2
			GIT_REPOSITORY https://github.com/GNOME/libxml2.git
			GIT_TAG v2.15.0
			PREFIX libxml2
			INSTALL_DIR ${LIB_DIR}/libxml2
			CMAKE_ARGS ${stdArgs}
				-DCMAKE_PREFIX_PATH=${LIB_DIR}
				-DLIBXML2_WITH_ZLIB=OFF
				-DLIBXML2_WITH_PYTHON=OFF
				-DLIBXML2_WITH_TESTS=OFF
				-DLIBXML2_WITH_ICONV=OFF
				-DLIBXML2_WITH_LZMA=OFF
				-DLIBXML2_WITH_THREADS=ON
				-DLIBXML2_BUILD_SHARED_LIBS=OFF
	)
	set( PC_LIBXML_INCLUDEDIR ${LIB_DIR}/libxml2/include/libxml2 CACHE PATH "the directory containing LibXml2 headers" )
	# set( LIBXML_INCLUDE_DIR ${LIB_DIR}/LibXml2/include/libxml2 CACHE PATH "the directory containing LibXml2 headers" )
	# set( LIBXML2_LIBRARY ${LIB_DIR}/LibXml2/lib CACHE PATH "the directory containing LibXml2 libraries" )
	set( PC_LIBXML_LIBDIR ${LIB_DIR}/libxml2/lib CACHE PATH "the directory containing LibXml2 libraries" )
	message( PC_LIBXML_INCLUDEDIR: ${PC_LIBXML_INCLUDEDIR} )
	message( PC_LIBXML_LIBDIR: ${PC_LIBXML_LIBDIR} )
else()
#need to add '#define _C4_STD_VECTOR_FWD_HPP_' to ryml_all.hpp to avoid redefinition errors
	ExternalProject_Add(jsonnet
		GIT_REPOSITORY https://github.com/google/jsonnet.git
		GIT_TAG "v0.21.0"
		PREFIX jsonnet
		INSTALL_DIR ${LIB_DIR}/jsonnet
		CMAKE_ARGS ${stdArgs}
			-DCMAKE_PREFIX_PATH=${LIB_DIR}
			-DUSE_SYSTEM_GTEST=ON
			-DBUILD_TESTS=OFF
	)
	ExternalProject_Add_StepDependencies( jsonnet configure gtest )
endif()
ExternalProject_Add(open62541
	GIT_REPOSITORY https://github.com/open62541/open62541.git
#	GIT_TAG v1.4.13
	PREFIX open62541
	INSTALL_DIR ${LIB_DIR}/open62541
		CMAKE_ARGS ${stdArgs}
		-DUA_ENABLE_ENCRYPTION=ON
		-DUA_ENABLE_ENCRYPTION_OPENSSL=ON
		-DOPENSSL_USE_STATIC_LIBS=ON
		-DOPENSSL_ROOT_DIR="${OPENSSL_ROOT_DIR}"
		-DBUILD_NODESET_LOADER=ON
		-DUA_ENABLE_XML_ENCODING=ON
)
ExternalProject_Add(open62541-nodeset-loader
	GIT_REPOSITORY https://github.com/open62541/open62541-nodeset-loader.git
#	GIT_TAG v0.5.0
	PREFIX nodeset-loader
	INSTALL_DIR ${LIB_DIR}/NodesetLoader
		CMAKE_ARGS ${stdArgs}
		-DENABLE_BACKEND_OPEN62541=OFF
		-DENABLE_EXAMPLES=OFF
		-DCMAKE_PREFIX_PATH=${LIB_DIR}
		-DPC_LIBXML_INCLUDEDIR=${LIB_DIR}/libxml2/include/libxml2
		-DPC_LIBXML_LIBDIR=${LIB_DIR}/libxml2/lib
		-DCMAKE_BUILD_TYPE=Debug
)

ExternalProject_Add_StepDependencies( spdlog configure fmt )
ExternalProject_Add_StepDependencies( absl configure gtest )
ExternalProject_Add_StepDependencies( protobuf configure absl )
ExternalProject_Add_StepDependencies( protobuf configure gtest )
ExternalProject_Add_StepDependencies( open62541-nodeset-loader configure open62541 )
if( WIN32 )
	ExternalProject_Add_StepDependencies( open62541-nodeset-loader configure LibXml2 )
endif()