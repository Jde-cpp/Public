#ifdef BOOST_ALL_NO_LIB
	#include <boost/json/src.hpp>
#endif
#include "gtest/gtest.h"
#include <jde/fwk/settings.h>
#include <jde/tests/testMain.h>
#define let const auto

namespace Jde{
#ifndef _MSC_VER
	α Process::ProductName()ι->sv{ return "Tests.AppServer"; }
#endif
 	Ω startup( int argc, char **argv )ι->void{
		Process::Startup( argc, argv, "Tests.AppServer", "AppServer unit tests", true );
		Logging::Init();
	}
}

α main( int argc, char **argv )->int{
	using namespace Jde;
	let filterSet = Process::Args().find("--gtest_filter") != Process::Args().end();
	::testing::InitGoogleTest( &argc, argv );
	startup( argc, argv );
	auto result = EXIT_FAILURE;
	{
		let p = Settings::FindSV( "/testing/tests" );
		let filter = p ? *p : "*";
		if( !filterSet ){
			INFOT( ELogTags::App, "filter:'{}'", filter );
			::testing::GTEST_FLAG( filter ) = filter;
		}
		result = CheckTestsRan( RUN_ALL_TESTS() );
		Process::Shutdown( result );
	}
	return result;
}
