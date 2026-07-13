cmake_path( SET jdeRoot NORMALIZE ${CMAKE_CURRENT_LIST_DIR}/.. )

if( CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR )
	message( FATAL_ERROR "In-source builds are not allowed. Configure from an out-of-source build directory, e.g.:\n  cd $JDE_BUILD_DIR/$JDE_COMPILER/<repo-name> && cmake ${CMAKE_SOURCE_DIR} --preset <preset>" )
endif()

if( CMAKE_HOST_WIN32 )
	set( CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>" )
	set( CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>" )
	set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>" )
	set( CMAKE_PDB_OUTPUT_DIRECTORY     "${CMAKE_BINARY_DIR}/bin/$<CONFIG>" )
endif()

function(boost)
	if( WIN32 )
		set( Boost_NO_WARN_NEW_VERSIONS ON )
		if( Boost_USE_STATIC_LIBS )
			find_package( Boost ${BOOST_VERSION} REQUIRED COMPONENTS json )
			include_directories( ${Boost_INCLUDE_DIRS} )
		else()
			set( _boostSrc $ENV{REPO_DIR}/boostorg/boost_1_91_0 )
			include_directories( ${_boostSrc} )
			add_compile_definitions( BOOST_ALL_NO_LIB=1 )
			if( NOT TARGET boost_json )
				add_library( boost_json STATIC ${_boostSrc}/libs/json/src/src.cpp )
				add_library( Boost::json ALIAS boost_json )
				target_include_directories( boost_json SYSTEM PUBLIC ${_boostSrc} )
			endif()
		endif()
	else()
		cmake_policy(SET CMP0167 NEW)
		find_package( Boost ${BOOST_VERSION} REQUIRED COMPONENTS json )
		include_directories( ${Boost_INCLUDE_DIRS} )
	endif()
endfunction()

# protobuf_generate(TARGET...)'s .pb.cc outputs don't exist yet at configure time (they're
# produced by a build-time custom command), so file(GLOB ${outDir}/*.pb.cc) would find nothing
# on a fresh checkout. Derive the expected output paths from the known .proto source list
# instead - set_source_files_properties doesn't require the file to exist yet.
function( suppressProtoWarnings protos outDir )
	if( NOT MSVC )
		set( _protoSources )
		foreach( _proto ${protos} )
			get_filename_component( _name ${_proto} NAME_WLE )
			list( APPEND _protoSources ${outDir}/${_name}.pb.cc )
		endforeach()
		set_source_files_properties( ${_protoSources} PROPERTIES COMPILE_OPTIONS "-Wno-nullability-extension;-Wno-invalid-offsetof" )
	endif()
endfunction()

# Symlinks src->dst as a proper build dependency (DEPENDS+OUTPUT) so it only
# reruns when src actually changes, instead of on every build like a
# TARGET-level PRE_BUILD/POST_BUILD custom command would.
function( linkGeneratedHeader targetName src dst )
	add_custom_command(
		OUTPUT ${dst}
		COMMAND ${CMAKE_COMMAND} -E create_symlink ${src} ${dst}
		DEPENDS ${src}
		COMMENT "mklink ${dst}"
	)
	target_sources( ${targetName} PRIVATE ${dst} )
endfunction()

function(dumpVariables)
	get_cmake_property(_variableNames VARIABLES)
	list (SORT _variableNames)
	foreach (_variableName ${_variableNames})
#        if ((NOT DEFINED ${ARGV0}) OR _variableName MATCHES ${ARGV0})
					message(STATUS "${_variableName}=${${_variableName}}")
#        endif()
	endforeach()
endfunction()

if( WIN32 )
	function( copyLibDlls )
		set( buildLibDir ${CMAKE_BINARY_DIR}/libs )
		add_custom_command( TARGET ${targetName} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${MULTI_INSTALL_PREFIX}/fmt/bin/fmt$<IF:$<CONFIG:Debug>,d,>.dll" $<TARGET_FILE_DIR:${targetName}>  COMMENT "fmtd.dll" )
		add_custom_command( TARGET ${targetName} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${TYPE_INSTALL_PREFIX}/zlib/bin/z$<IF:$<CONFIG:Debug>,d,>.dll" $<TARGET_FILE_DIR:${targetName}> COMMENT "copy z.dll" )
	endfunction()
	function( copyCommonDlls )
		copyLibDlls()
		set( buildLibDir ${CMAKE_BINARY_DIR}/libs )
	endfunction()
endif()

function(compileOptions)
	if( NOT MSVC )
		message( VERBOSE "compileOptions: ${ARGV0}" PRIVATE -Wall -Wextra -pedantic -Werror ${EXCLUDED_WARNINGS} )
		target_compile_options( ${ARGV0} PRIVATE -Wall -Wextra -pedantic -Werror ${EXCLUDED_WARNINGS} )
		if( NOT $ENV{OPTIMIZATION_LEVEL} STREQUAL "" )
			target_compile_options( ${ARGV0} PRIVATE -$ENV{OPTIMIZATION_LEVEL} )
		endif()
		set_property( TARGET ${ARGV0} PROPERTY POSITION_INDEPENDENT_CODE ON )
	endif()
endfunction()

#Native-proc MODULE for the sqlite driver - dlopen'd for sqlite_api.h's RegisterProcs( IProcs& ), never linked.
#Globs *.cpp/*.h from the calling directory plus any extra source dirs passed after the target name.
function( sqliteProcModule targetName )
	list( APPEND CMAKE_PREFIX_PATH ${MULTI_INSTALL_PREFIX}/sqlite ) #static lib built from the amalgamation - see libs/db/drivers/sqlite/sketch.md.
	find_package( SQLite3 REQUIRED )
	find_package( Threads REQUIRED )

	add_library( ${targetName} MODULE )
	compileOptions( ${targetName} )
	set_property( TARGET ${targetName} PROPERTY POSITION_INDEPENDENT_CODE ON )

	foreach( dir ${CMAKE_CURRENT_SOURCE_DIR} ${ARGN} )
		file( GLOB sources ${dir}/*.cpp )
		file( GLOB headers ${dir}/*.h )
		target_sources( ${targetName} PRIVATE ${sources} ${headers} )
	endforeach()

	target_link_libraries( ${targetName} PRIVATE SQLite::SQLite3 Threads::Threads ) #sqlite3 C API used directly (last_insert_rowid); the driver is reached only through IProcs at runtime.
	target_link_libraries( ${targetName} PRIVATE fmt::fmt Jde.DB )

	target_precompile_headers( ${targetName}
	  PRIVATE
		<sqlite3.h>
		<jde/fwk.h>
		<jde/fwk/str.h>
		<jde/fwk/io/json.h>
		<jde/fwk/chrono.h>
	)
endfunction()