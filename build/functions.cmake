cmake_path( SET jdeRoot NORMALIZE ${CMAKE_CURRENT_LIST_DIR}/.. )
#Note: file(GLOB) calls repo-wide deliberately omit CONFIGURE_DEPENDS - adding a new source file requires a manual reconfigure.
#Sole exception: sqliteProcModule (below).  Its targets are MODULEs, where a source missing from a stale glob still
#links - undefined symbols are legal - and only fails at dlopen; everywhere else a missed source is a link error.

if( CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR )
	message( FATAL_ERROR "In-source builds are not allowed. Configure from an out-of-source build directory, e.g.:\n  cd $JDE_BUILD_DIR/$JDE_COMPILER/<repo-name> && cmake ${CMAKE_SOURCE_DIR} --preset <preset>" )
endif()

#Nothing here uses modules (no `export module`, no `import`), but the C++26 standard level turns scanning on by
#default, costing a clang scan pass per TU plus a dyndep regen per target.  It also makes every object edge in a
#target depend on that target's single CXX.dd, so touching one source provisionally dirties all of them (a one-file
#change reads as a whole-target rebuild in ninja's progress count).  Must be set before any target is created -
#it seeds the CXX_SCAN_FOR_MODULES property at add_library/add_executable time.  Turn back on to adopt `import std`.
set( CMAKE_CXX_SCAN_FOR_MODULES OFF )

if( CMAKE_HOST_WIN32 )
	set( CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" )
	set( CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" )
	set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" )
	set( CMAKE_PDB_OUTPUT_DIRECTORY     "${CMAKE_BINARY_DIR}/bin" )
endif()

function(boost)
	if( WIN32 )
		set( Boost_NO_WARN_NEW_VERSIONS ON )
		set( _boostSrc $ENV{REPO_DIR}/boostorg/boost_1_91_0 )
		include_directories( ${_boostSrc} )
		add_compile_definitions( BOOST_ALL_NO_LIB=1 )
		if( NOT TARGET boost_json )
			add_library( boost_json STATIC ${_boostSrc}/libs/json/src/src.cpp )
			add_library( Boost::json ALIAS boost_json )
			target_include_directories( boost_json SYSTEM PUBLIC ${_boostSrc} )
		endif()
		#charconv is a compiled library too (the mysql driver's Boost.MySQL needs it).  There is no BoostConfig.cmake in
		#this source tree, so find_package( Boost COMPONENTS charconv ) cannot work here - build it like json above.
		if( NOT TARGET boost_charconv )
			add_library( boost_charconv STATIC ${_boostSrc}/libs/charconv/src/from_chars.cpp ${_boostSrc}/libs/charconv/src/to_chars.cpp )
			add_library( Boost::charconv ALIAS boost_charconv )
			target_include_directories( boost_charconv SYSTEM PUBLIC ${_boostSrc} )
		endif()
	else()
		cmake_policy(SET CMP0167 NEW)
		find_package( Boost REQUIRED COMPONENTS json )
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

#Configure-time symlink for static config files - idempotent; replaces the old POST_BUILD create_symlink steps that reran every build.
function( linkConfigFile src dst )
	file( REMOVE ${dst} )
	file( CREATE_LINK ${src} ${dst} SYMBOLIC )
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
		add_custom_command( TARGET ${targetName} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_INSTALL_PREFIX}/fmt/bin/fmt$<IF:$<CONFIG:Debug>,d,>.dll" $<TARGET_FILE_DIR:${targetName}>  COMMENT "fmtd.dll" )
		add_custom_command( TARGET ${targetName} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_INSTALL_PREFIX}/zlib/bin/z$<IF:$<CONFIG:Debug>,d,>.dll" $<TARGET_FILE_DIR:${targetName}> COMMENT "copy z.dll" )
	endfunction()
	#Stages the shared-library targets named in ARGN (+their pdbs) next to ${targetName}, for the targets whose
	#RUNTIME_OUTPUT_DIRECTORY is not <buildDir>/bin.  Defaults to `Jde Jde.DB` - the pair nearly every consumer needs -
	#so pass an explicit list to narrow it: the staging edge is a build-order dependency, so naming a dll the exe never
	#loads builds it for nothing (`--target Jde.Fwk.Tests` used to build Jde.DB that way).
	#Deliberately NOT a POST_BUILD step on ${targetName}: ninja lists bin/Jde.dll only as an order-only input (`||`) of the
	#consumer's link, and lld-link leaves the import lib byte-identical when the exported symbols don't change, so RESTAT
	#prunes the consumer's relink - a POST_BUILD command hanging off that link then silently never runs and leaves a stale
	#dll beside the exe (edit a function body, debug the old code).  An OUTPUT rule that DEPENDS on the dlls themselves is a
	#first-class edge that reruns whenever they are rewritten, relink or not.
	#The stamp exists because the destination cannot be the OUTPUT: for the targets that do live in bin, that path is
	#already the dll's own producing rule and ninja rejects the duplicate (the copy is then a no-op onto itself).
	#The pdbs are copied but kept out of DEPENDS - ninja knows no rule producing them, so listing them fails a clean tree.
	function( copyCommonDlls )
		copyLibDlls()
		set( dlls ${ARGN} )
		if( NOT dlls )
			set( dlls Jde Jde.DB )
		endif()
		foreach( dll ${dlls} )
			list( APPEND dllFiles $<TARGET_FILE:${dll}> )
			list( APPEND pdbFiles $<TARGET_PDB_FILE:${dll}> )
		endforeach()
		string( REPLACE ";" "/" dllNames "${dlls}" ) #COMMENT is one string: keep it readable instead of `Jde;Jde.DB`.
		set( stamp ${CMAKE_CURRENT_BINARY_DIR}/${targetName}.dlls.stamp )
		add_custom_command( OUTPUT ${stamp}
			COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${targetName}> #may not exist yet: this runs before ${targetName} links.
			COMMAND ${CMAKE_COMMAND} -E copy_if_different ${dllFiles} ${pdbFiles} $<TARGET_FILE_DIR:${targetName}>
			COMMAND ${CMAKE_COMMAND} -E touch ${stamp}
			DEPENDS ${dlls} ${dllFiles}
			COMMENT "copy ${dllNames} dlls -> ${targetName}"
		)
		add_custom_target( ${targetName}.dlls DEPENDS ${stamp} )
		add_dependencies( ${targetName} ${targetName}.dlls ) #staging depends on ${dlls}, not on ${targetName}, so this is not a cycle - and `--target ${targetName}` stages too.
	endfunction()
endif()

function(compileOptions)
	if( NOT MSVC )
		message( VERBOSE "compileOptions: ${ARGV0} -Wall -Wextra -pedantic -Werror ${EXCLUDED_WARNINGS}" )
		target_compile_options( ${ARGV0} PRIVATE -Wall -Wextra -pedantic -Werror ${EXCLUDED_WARNINGS} )
		if( NOT "$ENV{OPTIMIZATION_LEVEL}" STREQUAL "" )
			target_compile_options( ${ARGV0} PRIVATE -$ENV{OPTIMIZATION_LEVEL} )
		endif()
		set_property( TARGET ${ARGV0} PROPERTY POSITION_INDEPENDENT_CODE ON )
	endif()
endfunction()

#Registers targetName with ctest: runs from ${CMAKE_BINARY_DIR}/Testing with the env vars the jsonnet
#configs expand via $(REPO_SOURCE_DIR)/$(REPO_BUILD_DIR); extra COMMAND args can follow the settings file.
#`-include=args/sqlite -arg path=:memory:` is the default so every ctest run is self-contained (in-memory sqlite, no
#db server): the db-backed suites need it as a pair, and fwk/web/sqlite-driver import no args dir and don't read
#`path`, so it is inert for them.  Extra args in ARGN follow it.
function( addJdeTest targetName settingsFile )
	add_test(
		NAME ${targetName}
		COMMAND $<TARGET_FILE:${targetName}> -ctest -settings=${settingsFile} -include=args/sqlite -arg path=:memory: ${ARGN}
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/Testing
	)
	set_tests_properties( ${targetName} PROPERTIES ENVIRONMENT
		"REPO_SOURCE_DIR=${CMAKE_SOURCE_DIR};REPO_BUILD_DIR=${CMAKE_BINARY_DIR}/.."
	)
endfunction()

#Build-order dependency on the sqlite driver plus the native-proc MODULEs targetName dlopen's at runtime from the
#paths in its jsonnet.  They are never linked, so without this nothing makes the build produce them and the run dies
#in DB::DataSource with "Dynamic Library ... not found".  Jde.DB.Sqlite is implied - a proc MODULE is only reachable
#through it.  Deliberately unguarded: add_dependencies resolves at generate time, so naming a target defined by a
#later add_subdirectory is fine, whereas an `if( TARGET )` guard would evaluate now and silently drop it.
function( sqliteProcDependencies targetName )
	add_dependencies( ${targetName} Jde.DB.Sqlite ${ARGN} )
endfunction()

#Native-proc MODULE for the sqlite driver - dlopen'd for sqlite_api.h's RegisterProcs( IProcs& ), never linked.
#Globs *.cpp/*.h from the calling directory plus any extra source dirs passed after the target name.
function( sqliteProcModule targetName )
	find_package( Threads REQUIRED )

	add_library( ${targetName} MODULE )
	compileOptions( ${targetName} )
	set_property( TARGET ${targetName} PROPERTY POSITION_INDEPENDENT_CODE ON )
	#RegisterProcs is exported from source via JDE_SQLITE_PROC in <jde/db/sqlite_api.h> - no /EXPORT: link flag needed.

	#CONFIGURE_DEPENDS: the module is a MODULE, so a new proc twin that isn't in the glob still links (undefined
	#symbols are legal) and only fails at dlopen - re-glob on build instead of making that a reconfigure-or-else.
	foreach( dir ${CMAKE_CURRENT_SOURCE_DIR} ${ARGN} )
		file( GLOB sources CONFIGURE_DEPENDS ${dir}/*.cpp )
		file( GLOB headers CONFIGURE_DEPENDS ${dir}/*.h )
		target_sources( ${targetName} PRIVATE ${sources} ${headers} )
	endforeach()

	target_link_libraries( ${targetName} PRIVATE Threads::Threads ) #no sqlite3 link: the driver is reached only through IProcs (sqlite_api.h), which forward-declares sqlite3.
	target_link_libraries( ${targetName} PRIVATE fmt::fmt Jde.DB ) #PUBLIC-links Jde on WIN32, propagated transitively.

	target_precompile_headers( ${targetName}
	  PRIVATE
		<jde/fwk.h>
		<jde/fwk/str.h>
		<jde/fwk/io/json.h>
		<jde/fwk/chrono.h>
	)
endfunction()