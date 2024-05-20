cmake_minimum_required(VERSION 3.16)
project(Jde.Iot)
set( CMAKE_MODULE_PATH $ENV{JDE_DIR}/Public/build )
include(common)
link_directories( ${fmt_DIR}/../.. )
link_directories( ${Protobuf_DIR}/../.. )
#######################################################
set(CMAKE_SHARED_LINKER_FLAGS ${CMAKE_SHARED_LINKER_FLAGS} "-Wl,-rpath=$ORIGIN")
add_definitions( -DJde_Iot_EXPORTS -DUA_ENABLE_ENCRYPTION )

find_package(OpenSSL REQUIRED)
find_package(open62541 REQUIRED)
set( open62541Dir ${open62541_DIR}/../../.. )
#message( "open62541Dir=${open62541Dir}")
link_directories( ${open62541Dir}/lib )
include_directories( ${open62541Dir}/include )

add_library( ${CMAKE_PROJECT_NAME} SHARED 
	UM.cpp
	async/AsyncRequest.cpp async/Attributes.cpp async/CreateSubscriptions.cpp async/ConnectAwait.cpp async/DataChanges.cpp async/SessionAwait.cpp async/SetMonitoringMode.cpp async/Write.cpp
	types/MonitoringNodes.cpp	types/OpcServer.cpp
	uatypes/Browse.cpp uatypes/Logger.cpp uatypes/Node.cpp uatypes/Value.cpp uatypes/UAClient.cpp
	types/proto/IotCommon.pb.cc types/proto/IotFromClient.pb.cc types/proto/IotFromServer.pb.cc )
#######################################################
#target_link_libraries( ${CMAKE_PROJECT_NAME} Threads::Threads )
target_link_libraries( ${CMAKE_PROJECT_NAME} debug libfmtd.a optimized libfmt.a )
target_link_libraries(${CMAKE_PROJECT_NAME} OpenSSL::SSL)
target_link_libraries( ${CMAKE_PROJECT_NAME} protobuf::libprotobuf )
target_link_libraries( ${CMAKE_PROJECT_NAME} libopen62541.a )
#######################################################
target_precompile_headers(${CMAKE_PROJECT_NAME}
  PRIVATE
		<boost/lexical_cast.hpp>
		<boost/unordered/concurrent_flat_map.hpp>
		<boost/unordered/concurrent_flat_set.hpp>
		<boost/uuid/uuid.hpp>
		<boost/uuid/uuid_io.hpp>
		<open62541/client_config_default.h>
		<open62541/client_highlevel.h>
		<open62541/client_highlevel_async.h>
		<open62541/client_subscriptions.h>
		<open62541/plugin/log_stdout.h>
		<jde/iot/TypeDefs.h>
		<jde/Exports.h>
		<jde/Log.h>
		<jde/App.h>
		<jde/Str.h>
		<jde/coroutine/Task.h>
		<jde/crypto/OpenSsl.h>
		"../web/Exports.h"
		"../../../Framework/source/DateTime.h"
		"../../../Framework/source/coroutine/Alarm.h"
		"../../../Framework/source/coroutine/Awaitable.h"
		"../../../Framework/source/db/Database.h"
		<jde/iot/Exports.h>
		<jde/iot/types/proto/IotCommon.pb.h>
		<jde/iot/types/proto/FromServer.pb.h>
		<jde/iot/types/proto/IotFromServer.pb.h>
		<jde/iot/types/proto/IotFromClient.pb.h>
		<jde/iot/uatypes/helpers.h>
		<jde/iot/uatypes/UAException.h>
)
