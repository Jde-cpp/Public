﻿#include "gtest/gtest.h"
#include "../../../Framework/source/Settings.h"
#include "../../../Framework/source/Cache.h"
#include "../../../Framework/source/db/GraphQL.h"
#include "helpers.h"
#define var const auto

namespace Jde{
	α OSApp::ProductName()ι->sv{ return "Tests.Iot"; }

 	α Startup( int argc, char **argv )ι->void{
#ifdef _MSC_VER
		ASSERT( Settings::Get<uint>("workers/drive/threads")>0 )
#endif
		ASSERT( argc>1 && string{argv[1]}=="-c" )
		OSApp::Startup( argc, argv, "Tests.Iot", "Iot tests" );
		DB::CreateSchema();
		DB::SetQLDataSource( DB::DataSourcePtr() );
		Iot::AddHook();
	}
}

α main( int argc, char **argv )->int{
	using namespace Jde;
	::testing::InitGoogleTest( &argc, argv );
	Startup( argc, argv );
	int result;
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