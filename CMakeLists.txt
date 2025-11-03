cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( build/functions.cmake )
project( Superbuild VERSION ${JDE_VERSION} LANGUAGES CXX )

add_subdirectory( build repos )
add_subdirectory( apps/AppServer )
add_subdirectory( apps/OpcServer )
add_subdirectory( apps/OpcGateway )

