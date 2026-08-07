//Jde.Opc's own unit suite:  the uatypes wrappers and their round trips - ToJson/FromJson, ToUAJson/ToUAValues,
//ToString/DecodeJson - over every identifier kind and array shape.  Nothing here starts a server, opens a socket or
//touches a data source; apps/OpcGateway/tests (confusingly, the target named Jde.Opc.Tests) is the integration suite.
//
//Convention:  a test named DISABLED_R2_<n>_* asserts the *correct* behaviour of finding #<n> in reviews/opc-review2.md.
//They are disabled rather than absent so the suite stays green while the findings are open - drop the DISABLED_ prefix
//as each one is fixed and the test becomes its acceptance check.  Do not enable one speculatively:  #4, #5 and #12
//corrupt the heap or std::terminate the runner as the library stands today.
#ifdef BOOST_ALL_NO_LIB
	#include <boost/json/src.hpp>
#endif
#include "gtest/gtest.h"
#include <jde/fwk/process/process.h>
#include <jde/fwk/settings.h>
#include <jde/opc/uatypes/Logger.h>
#include <jde/tests/testMain.h>

#define let const auto

namespace Jde{
#ifndef _MSC_VER
	α Process::ProductName()ι->sv{ return "Tests.Opc.Lib"; }
#endif
	Ω startup( int argc, char **argv )ε->void{
		Logging::AddTagParser( mu<Opc::UALogParser>() ); //before Startup:  the jsonnet names opc tags, and UALogParserTests asserts they resolve.
		Process::Startup( argc, argv, "Tests.Opc.Lib", "Jde.Opc uatypes unit tests", true );
		Logging::Init();
	}
}

α main( int argc, char **argv )->int{
	using namespace Jde;
	let filterSet = Process::Args().find( "--gtest_filter" )!=Process::Args().end();
	::testing::InitGoogleTest( &argc, argv );
	int exitCode{ EXIT_FAILURE };
	try{
		startup( argc, argv );
		if( !filterSet )
			::testing::GTEST_FLAG( filter ) = Settings::FindSV( "/testing/tests" ).value_or( "*" );
		exitCode = CheckTestsRan( RUN_ALL_TESTS() );
	}
	catch( exception& e ){
		if( auto p = dynamic_cast<Exception*>(&e); p ){
			p->Log();
			exitCode = p->HasCode() ? (int)p->Code() : EXIT_FAILURE;
		}
		std::cerr << e.what() << std::endl;
	}
	Process::Shutdown( exitCode );
	return exitCode;
}
