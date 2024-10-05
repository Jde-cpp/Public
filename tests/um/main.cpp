#include "gtest/gtest.h"
#include <jde/App.h>
#include <jde/um/UM.h>
// #include "../../../Framework/source/Settings.h"
// #include "../../../Framework/source/Cache.h"
// #include "../../../Framework/source/threading/Thread.h"
#include <jde/db/Database.h>
#define var const auto

namespace Jde{
	α OSApp::ProductName()ι->sv{ return "Tests.UM"; }

 	α Startup( int argc, char **argv )ι->void{
#ifdef _MSC_VER
		ASSERT( Settings::Get<uint>("workers/drive/threads")>0 )
#endif
		ASSERT( argc>1 && string{argv[1]}=="-c" )
		Threading::SetThreadDscrptn( "Main" );
		OSApp::Startup( argc, argv, OSApp::ProductName(), "UM tests" );
		if( Settings::Get<bool>("testing/recreateDB") ){
			var db{ "test_jde_um"s };
			auto connectionString = Str::Replace( *Settings::Env("db/connectionString"), db, "sys" );
			auto dataSource = DB::CreateDataSource( connectionString );
			dataSource->Execute( Ƒ("DROP DATABASE IF EXISTS {}", db) );
			dataSource->Execute( Ƒ("CREATE DATABASE IF NOT EXISTS {}", db) );
			UM::Configure();
		}
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
		Process::Shutdown( result );
	}
	return result;
}