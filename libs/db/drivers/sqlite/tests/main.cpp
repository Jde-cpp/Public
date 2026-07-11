#ifdef BOOST_ALL_NO_LIB
	#include <boost/json/src.hpp>
#endif
#include "gtest/gtest.h"
#include <jde/fwk/process/process.h>
#include <jde/fwk/settings.h>

#define let const auto

namespace Jde{
#ifndef _MSC_VER
	α Process::ProductName()ι->sv{ return "Tests.DB.Sqlite"; }
#endif
	Ω startup( int argc, char **argv )ε->void{
		Process::Startup( argc, argv, Process::ProductName(), "Sqlite driver tests", true );
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
		exitCode = RUN_ALL_TESTS();
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
