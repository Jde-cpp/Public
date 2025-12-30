#cls;rm -f CMakeCache.txt;cmake $JDE_DIR/build --preset linux-debug
cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( ExternalProject )
include( ${CMAKE_CURRENT_SOURCE_DIR}/functions.cmake )
project( repos VERSION ${JDE_VERSION} LANGUAGES CXX )

set( stdArgs
	-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
	-DCMAKE_CXX_FLAGS_DEBUG=${CMAKE_CXX_FLAGS_DEBUG}
	-DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
	-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
	-DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
	-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
)
if( isMultiConfig )
	set( stdArgs ${stdArgs} -DCMAKE_CONFIGURATION_TYPES=${CMAKE_BUILD_TYPE} )
endif()
if( WIN32 )
	set( stdArgs ${stdArgs} -DCMAKE_MSVC_RUNTIME_LIBRARY=${CMAKE_MSVC_RUNTIME_LIBRARY} )
else()
	set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-rpath=$ORIGIN" )
	set( stdArgs ${stdArgs} -DCMAKE_POSITION_INDEPENDENT_CODE=ON )
endif()

message( CMAKE_CONFIGURATION_TYPES=${CMAKE_CONFIGURATION_TYPES} )
ExternalProject_Add(fmt
	GIT_REPOSITORY https://github.com/fmtlib/fmt.git
	GIT_TAG 12.0.0
	PREFIX fmt
	INSTALL_DIR ${MULTI_INSTALL_PREFIX}/fmt
	CMAKE_ARGS ${stdArgs}
		-DBUILD_SHARED_LIBS=ON
		-DFMT_MASTER_PROJECT=OFF
		-DFMT_DOC=OFF
		-DFMT_INSTALL=ON
#-DCMAKE_POSITION_INDEPENDENT_CODE
)

ExternalProject_Add(gtest
	GIT_REPOSITORY https://github.com/google/googletest.git
	GIT_TAG v1.17.0
	PREFIX gtest
	INSTALL_DIR ${TYPE_INSTALL_PREFIX}/gtest
	LOG_CONFIGURE ON
  LOG_BUILD ON
	CMAKE_C_COMPILER_WORKS ON
	CMAKE_ARGS ${stdArgs}
		-DCMAKE_C_COMPILER_WORKS=ON
		-DBUILD_GMOCK=OFF
)

ExternalProject_Add(spdlog
	GIT_REPOSITORY https://github.com/gabime/spdlog.git
	GIT_TAG v1.16.0
	PREFIX spdlog
	INSTALL_DIR ${MULTI_INSTALL_PREFIX}/spdlog
	CMAKE_ARGS ${stdArgs}
		-DCMAKE_PREFIX_PATH=${MULTI_INSTALL_PREFIX}
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
	INSTALL_DIR ${TYPE_INSTALL_PREFIX}/absl
	CMAKE_ARGS ${stdArgs}
		-DCMAKE_PREFIX_PATH=${CMAKE_INSTALL_PREFIX}
		-DABSL_BUILD_TESTING=OFF
		-DABSL_USE_EXTERNAL_GOOGLETEST=ON
		#-DABSL_FIND_GOOGLETEST=ON
		-DABSL_PROPAGATE_CXX_STD=ON
)

IF( WIN32 )
  message( MULTI_INSTALL_PREFIX=${MULTI_INSTALL_PREFIX} )
	ExternalProject_Add(zlib
		GIT_REPOSITORY https://github.com/madler/zlib
		GIT_TAG "v1.3.1"
		PREFIX ZLIB
		INSTALL_DIR ${MULTI_INSTALL_PREFIX}/zlib
		CMAKE_ARGS ${stdArgs}
			-DZLIB_BUILD_SHARED=OFF
			-DZLIB_BUILD_TESTING=OFF
	)
	#message( MULTI_INSTALL_PREFIX=${MULTI_INSTALL_PREFIX}/zlib/lib )
	# set( ZLIB_LIBRARY ${MULTI_INSTALL_PREFIX}/zlib/lib CACHE PATH "the directory containing zlib libraries" )
	#message( ZLIB_LIBRARY=${ZLIB_LIBRARY} )
	# set( ZLIB_INCLUDE_DIR ${MULTI_INSTALL_PREFIX}/zlib/include CACHE PATH "the directory containing zlib headers" )

	ExternalProject_Add(
		LibXml2
		GIT_REPOSITORY https://github.com/GNOME/libxml2.git
		GIT_TAG v2.15.0
		PREFIX libxml2
		INSTALL_DIR ${MULTI_INSTALL_PREFIX}/libxml2
		CMAKE_ARGS ${stdArgs}
			-DCMAKE_PREFIX_PATH=${MULTI_INSTALL_PREFIX}
			-DLIBXML2_WITH_ZLIB=OFF
			-DLIBXML2_WITH_PYTHON=OFF
			-DLIBXML2_WITH_TESTS=OFF
			-DLIBXML2_WITH_ICONV=OFF
			-DLIBXML2_WITH_LZMA=OFF
			-DLIBXML2_WITH_THREADS=ON
			-DLIBXML2_BUILD_SHARED_LIBS=OFF
	)
	set( LIBXML2_INCLUDE_DIR ${MULTI_INSTALL_PREFIX}/libxml2/include/libxml2 CACHE PATH "the directory containing LibXml2 headers" )
	#set( LIBXML2_LIBRARY ${MULTI_INSTALL_PREFIX}/libxml2/lib/libxml2$<IF:$<CONFIG:Debug>,d,>.lib CACHE PATH "the directory containing LibXml2 libraries" )
	#set( LIBXML2_LIBRARY ${MULTI_INSTALL_PREFIX}/libxml2/lib/libxml2.lib CACHE PATH "the directory containing LibXml2 libraries" )
	#set( LIBXML2_LIBRARIES ${MULTI_INSTALL_PREFIX}/libxml2/lib CACHE PATH "the directory containing LibXml2 libraries" )
	#set( LIBXML2_LIBRARY ${MULTI_INSTALL_PREFIX}/libxml2/lib CACHE PATH "the directory containing LibXml2 libraries" )
else()
#need to add '#define _C4_STD_VECTOR_FWD_HPP_' to ryml_all.hpp to avoid redefinition errors
	ExternalProject_Add(jsonnet
		GIT_REPOSITORY https://github.com/google/jsonnet.git
		GIT_TAG "v0.21.0"
		PREFIX jsonnet
		INSTALL_DIR ${MULTI_INSTALL_PREFIX}/jsonnet
		CMAKE_ARGS ${stdArgs}
			-DCMAKE_PREFIX_PATH=${MULTI_INSTALL_PREFIX}
			-DUSE_SYSTEM_GTEST=ON
			-DBUILD_JSONNETFMT=OFF
			-DBUILD_TESTS=OFF
	)
	ExternalProject_Add_StepDependencies( jsonnet configure gtest )
endif()
ExternalProject_Add(protobuf
	GIT_REPOSITORY https://github.com/protocolbuffers/protobuf.git
	GIT_TAG v33.2
	PREFIX protobuf
	INSTALL_DIR ${MULTI_INSTALL_PREFIX}/protobuf
	CMAKE_ARGS ${stdArgs}
		-DCMAKE_PREFIX_PATH=${TYPE_INSTALL_PREFIX}
		-Dprotobuf_USE_EXTERNAL_GTEST=ON
		-Dprotobuf_BUILD_TESTS=OFF
		-Dprotobuf_ABSL_PROVIDER=package
		-Dprotobuf_LOCAL_DEPENDENCIES_ONLY=ON
		-Dprotobuf_MSVC_STATIC_RUNTIME=OFF
)

ExternalProject_Add(open62541
	GIT_REPOSITORY https://github.com/open62541/open62541.git
#	GIT_TAG v1.4.13
	PREFIX open62541
	INSTALL_DIR ${MULTI_INSTALL_PREFIX}/open62541
		CMAKE_ARGS ${stdArgs}
		-DUA_ENABLE_ENCRYPTION=ON
		-DUA_ENABLE_ENCRYPTION_OPENSSL=ON
		-DOPENSSL_USE_STATIC_LIBS=ON
		-DOPENSSL_ROOT_DIR="${OPENSSL_ROOT_DIR}"
		-DBUILD_NODESET_LOADER=ON
		-DUA_ENABLE_XML_ENCODING=ON
		-DUA_NAMESPACE_ZERO=FULL
		-DUA_BUILD_EXAMPLES=OFF
)

ExternalProject_Add(open62541-nodeset-loader
	GIT_REPOSITORY https://github.com/open62541/open62541-nodeset-loader.git
#	GIT_TAG v0.5.0
	PREFIX nodeset-loader
	INSTALL_DIR ${MULTI_INSTALL_PREFIX}/NodesetLoader
		CMAKE_ARGS ${stdArgs}
		-DENABLE_BACKEND_OPEN62541=OFF
		-DENABLE_EXAMPLES=OFF
		-DCMAKE_PREFIX_PATH=${MULTI_INSTALL_PREFIX}
#linux
		-DLIBXML2_INCLUDE_DIR=${LIBXML2_INCLUDE_DIR}
#		-DPC_LIBXML_LIBDIR=${LIBXML2_LIBDIR}
#win
#		-DLIBXML2_LIBRARIES=${LIBXML2_LIBRARIES}
#		-DLIBXML2_INCLUDE_DIR=${LIBXML2_INCLUDE_DIR}
		-DCMAKE_BUILD_TYPE=Debug

)

ExternalProject_Add_StepDependencies( spdlog configure fmt )
ExternalProject_Add_StepDependencies( absl configure gtest )
ExternalProject_Add_StepDependencies( protobuf configure absl )
ExternalProject_Add_StepDependencies( protobuf configure gtest )
ExternalProject_Add_StepDependencies( open62541-nodeset-loader configure open62541 )
if( WIN32 )
	ExternalProject_Add_StepDependencies( protobuf configure zlib )
#	ExternalProject_Add_StepDependencies( open62541-nodeset-loader configure LibXml2 )
endif()