#include "gtest/gtest.h"
#include "../../../Framework/source/Settings.h"
#include "../../../Framework/source/Cache.h"
#define var const auto

namespace Jde{
	α OSApp::ProductName()ι->sv{ return "Tests.Crypto"; }

 	α Startup( int argc, char **argv )ι->void{
#ifdef _MSC_VER
		ASSERT( Settings::Get<uint>("workers/drive/threads")>0 )
#endif
		ASSERT( argc>1 && string{argv[1]}=="-c" )
		OSApp::Startup( argc, argv, "Tests.Crypto", "Crypto tests" );
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
		IApplication::Shutdown();
		IApplication::Cleanup();
	}
	return result;
}