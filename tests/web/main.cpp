#include "gtest/gtest.h"
#include "../../../Framework/source/Settings.h"
#include "../../../Framework/source/Cache.h"
#include "../../../Framework/source/threading/Thread.h"
#define var const auto

namespace Jde{
 	α Startup( int argc, char **argv )ι->void{
#ifdef _MSC_VER
		ASSERT( Settings::Get<uint>("workers/drive/threads")>0 )
#endif
		OSApp::SetProductName( "Tests.Web" );
		ASSERT( argc>1 && string{argv[1]}=="-c" )
		Threading::SetThreadDscrptn( "Main" );
		OSApp::Startup( argc, argv, "Tests.Web", "Web tests" );
	}
}

α main( int argc, char **argv )->int{
	using namespace Jde;
	::testing::InitGoogleTest( &argc, argv );
	Startup( argc, argv );
	auto result = EXIT_FAILURE;
	{
		var p=Settings::Get<string>( "testing/tests" );
		var filter = p ? *p : "*";
		::testing::GTEST_FLAG( filter ) = filter;
	   result = RUN_ALL_TESTS();
		IApplication::Shutdown( result );
		IApplication::Cleanup();
	}
	return result;
}