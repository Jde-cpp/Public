#include "gtest/gtest.h"
#include <jde/framework/settings.h>
//#include "../../../Framework/source/Cache.h"
#include "../../../../Framework/source/threading/Thread.h"
#define let const auto

namespace Jde{
#ifndef _MSC_VER
	α OSApp::ProductName()ι->sv{ return "Tests.Crypto"; }
#endif
 	α Startup( int argc, char **argv )ι->void{
#ifdef _MSC_VER
		ASSERT( Settings::FindNumber<uint>("workers/drive/threads").value_or(0)>0 )
#endif
		Threading::SetThreadDscrptn( "Main" );
		OSApp::Startup( argc, argv, "Tests.Crypto", "Crypto tests", true );
	}
}

α main( int argc, char **argv )->int{
	using namespace Jde;
	let filterSet = Process::Args().find("--gtest_filter")!= Process::Args().end();
	::testing::InitGoogleTest( &argc, argv );
	Startup( argc, argv );
	auto result = EXIT_FAILURE;
	{
		let p=Settings::FindSV( "testing/tests" );
		let filter = p ? *p : "*";
		if( !filterSet ){
			Information{ ELogTags::App, "filter:'{}'", filter };
			::testing::GTEST_FLAG( filter ) = filter;
		}
		result = RUN_ALL_TESTS();
		Process::Shutdown( result );
	}
	return result;
}