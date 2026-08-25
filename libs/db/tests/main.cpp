#include "gtest/gtest.h"
#include <jde/fwk/process/process.h>
#include <jde/fwk/settings.h>
#include <jde/tests/testMain.h>

#define let const auto

namespace Jde{
#ifndef _MSC_VER
	α Process::ProductName()ι->sv{ return "Tests.DB"; }
#endif
	Ω startup( int argc, char **argv )ε->void{
		Process::Startup( argc, argv, Process::ProductName(), "DB generator/value tests", true );
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
	catch( runtime_error& e ){
		exitCode = StartupFailed( e );//never the exception's code:  main's status keeps only its low 8 bits.
	}
	Process::Shutdown( exitCode );
	return exitCode;
}
