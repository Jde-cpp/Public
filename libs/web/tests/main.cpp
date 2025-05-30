#include "gtest/gtest.h"
#include "../../../../Framework/source/threading/Thread.h"
#define let const auto

namespace Jde{
#ifndef _MSC_VER
	α OSApp::ProductName()ι->sv{ return "Tests.Web"; }
#endif

 	α Startup( int argc, char **argv )ι->void{
#ifdef _MSC_VER
		ASSERT( Settings::FindNumber<uint>("/workers/drive/threads").value_or(0)>0 )
#endif
		Threading::SetThreadDscrptn( "Main" );
		OSApp::Startup( argc, argv, "Tests.Web", "Web tests", true );
	}
}

α main( int argc, char **argv )->int{
	using namespace Jde;
	::testing::InitGoogleTest( &argc, argv );
	Startup( argc, argv );
	auto result = EXIT_FAILURE;
	{
		let filter=Settings::FindSV( "/testing/tests" ).value_or( "*" );
		::testing::GTEST_FLAG( filter ) = filter;
	   result = RUN_ALL_TESTS();
		Process::Shutdown( result );
	}
	return result;
}