#include "gtest/gtest.h"
#include <jde/framework/settings.h>
#include <jde/framework/process/thread.h>
#define let const auto

namespace Jde{
#ifndef _MSC_VER
	α Process::ProductName()ι->sv{ return "Tests.Crypto"; }
#endif
 	α Startup( int argc, char **argv )ι->void{
		SetThreadDscrptn( "Main" );
		Process::Startup( argc, argv, "Tests.Crypto", "Crypto tests", true );
	}
}

α main( int argc, char **argv )->int{
	using namespace Jde;
	let filterSet = Process::Args().find("--gtest_filter") != Process::Args().end();
	::testing::InitGoogleTest( &argc, argv );
	Startup( argc, argv );
	auto result = EXIT_FAILURE;
	{
		let p=Settings::FindSV( "testing/tests" );
		let filter = p ? *p : "*";
		if( !filterSet ){
			INFOT( ELogTags::App, "Test Filter:'{}'", filter );
			::testing::GTEST_FLAG( filter ) = filter;
		}
		result = RUN_ALL_TESTS();
		Process::Shutdown( result );
	}
	return result;
}