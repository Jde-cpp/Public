boost()
if( WIN32 )
	include_directories( ${OPENSSL_ROOT_DIR}/include )
	link_directories( ${OPENSSL_CRYPTO_LIBRARY_DIR} )
else()
	find_package( OpenSSL REQUIRED )
endif()

include_directories( $ENV{JDE_DIR}/include )

find_package( fmt REQUIRED )

find_package( spdlog REQUIRED )
include_directories( ${spdlog_DIR}/../../../include ) #${spdlog_INCLUDE_DIRS}

if( WIN32 )
	set( JSONNET_DIR $ENV{REPO_DIR}/jsonnet CACHE PATH "Jsonnet directory" )
	include_directories( ${JSONNET_DIR}/include )
	link_directories( ${JSONNET_LIB_DIR} )
else()
	set( JSONNET_DIR ${TYPE_INSTALL_PREFIX}/jsonnet CACHE PATH "Jsonnet directory" )
	include_directories( ${JSONNET_DIR}/include )
	link_directories( ${JSONNET_DIR}/lib )
endif()
