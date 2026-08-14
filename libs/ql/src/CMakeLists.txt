cmake_minimum_required( VERSION ${MIN_REQ_CMAKE_VERSION} )
include( ${CMAKE_CURRENT_LIST_DIR}/../../../build/functions.cmake )
set( targetName Jde.QL )
project( ${targetName} VERSION ${JDE_VERSION} LANGUAGES CXX )
include( ${jdeRoot}/build/dependencies.cmake )
add_library( ${targetName} )
compileOptions( ${targetName} )
file( GLOB sources *.cpp */*.cpp )
file( GLOB headers *.h */*.h ${jdeRoot}/include/jde/ql/*.h ${jdeRoot}/include/jde/ql/ops/*.h ${jdeRoot}/include/jde/ql/types/*.h )
target_sources(${targetName}
	PRIVATE
		${sources}
		${headers}
)

target_link_libraries( ${targetName} Jde Jde.DB )
target_link_libraries( ${targetName} Boost::json )

#FilterQL's `regex:` operator matches client-supplied patterns per row, on the log fan-out thread, so the match has to
#be bounded.  boost::regex bounds it three ways; this is the one that is tunable.  Measured, it is *not* the one that
#catches the classic bombs - a recursion-depth guard (perl_matcher_non_recursive.hpp:205, m_recursions>80) stops every
#`(a+)+$`-shaped pattern in ~1ms at any cap.  This bounds the shallow-but-wide shape that guard does not see, and 100k
#costs nothing: legitimate patterns are unaffected up to 200KB subjects (~0.5ms), verified against boost's own default
#of 100,000,000.  A -D, not a #define in FilterQL.cpp: the PUBLIC precompiled header below is force-included ahead of
#the file's first line, so a #define there could land after boost/regex/config.hpp had already baked the default in.
#FilterQL.cpp static_asserts the value, so dropping this line is a compile error, not a silent widening.
#Boost.Regex v5 is header-only (libs/regex/src holds only the POSIX C wrappers), so there is nothing to link.
target_compile_definitions( ${targetName} PRIVATE BOOST_REGEX_MAX_STATE_COUNT=100000 )

target_precompile_headers(${targetName}
  PUBLIC
  <jde/fwk.h>
)