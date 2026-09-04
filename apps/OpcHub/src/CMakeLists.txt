cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( ${CMAKE_CURRENT_LIST_DIR}/../../../build/functions.cmake )
message( "Jde.Opc.HubLib" )
project( Jde.Opc.HubLib VERSION ${JDE_VERSION} LANGUAGES CXX )
include( ${jdeRoot}/build/dependencies.cmake )

#The composition of Jde.App.ServerLib + Jde.Opc.GatewayLib into one process (hubStartup.cpp) and the gateway's in-process
#app client (HubAppClient.cpp).  A library so Jde.Opc.Hub.Tests embeds exactly what the exe runs, as the two app libs do.
set( targetName Jde.Opc.HubLib )
add_library( ${targetName} )
compileOptions( ${targetName} )
file( GLOB sources CONFIGURE_DEPENDS *.cpp ql/*.cpp )
get_filename_component( main_cpp ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp ABSOLUTE )
list( REMOVE_ITEM sources ${main_cpp} )
file( GLOB headers CONFIGURE_DEPENDS *.h ql/*.h )
target_sources( ${targetName} PRIVATE ${sources} ${headers} )

add_dependencies( ${targetName} Jde.DB.MySql ) #built on every platform; odbc only exists on windows.
if( WIN32 )
	add_dependencies( ${targetName} Jde.DB.Odbc )
endif()
sqliteProcDependencies( ${targetName} Jde.DB.Sqlite.AppServer Jde.DB.Sqlite.OpcGateway ) #config/args/sqlite dlopens both - transitive through the two libs, stated for the record.

target_include_directories( ${targetName} PRIVATE ${jdeRoot}/include/jde/app/proto )
target_link_libraries( ${targetName} Jde.App.ServerLib Jde.Opc.GatewayLib )

#The gateway lib's header set (open62541, the Opc protos) is what hubStartup/HubAppClient pull in through the gateway's
#headers; its pc.h is private to that directory, so this one is a copy of the parts that are reachable from here.
target_precompile_headers( ${targetName} PRIVATE pc.h )
