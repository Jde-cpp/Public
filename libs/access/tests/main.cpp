#include "gtest/gtest.h"
#include <jde/framework/process.h>
#include <jde/framework/settings.h>
#include <jde/db/db.h>
#include <jde/access/Access.h>
#define let const auto

namespace Jde{
	α OSApp::ProductName()ι->sv{ return "Tests.UM"; }

 	α Startup( int argc, char **argv )ι->void{
#ifdef _MSC_VER
		ASSERT( Settings::Get<uint>("workers/drive/threads")>0 )
#endif
		ASSERT( argc>1 && string{argv[1]}=="-c" )
		OSApp::Startup( argc, argv, OSApp::ProductName(), "UM tests" );

		let metaDataName{ "access" };
		auto schema = DB::GetSchema( metaDataName );
		if( Settings::Find<bool>("testing/recreateDB").value_or(false) )
			DB::NonProd::Recreate( schema );
		Access::Configure( schema );
	}
}

α main( int argc, char **argv )->int{
	using namespace Jde;
	::testing::InitGoogleTest( &argc, argv );
	Startup( argc, argv );
	auto result = EXIT_FAILURE;
	{
		::testing::GTEST_FLAG( filter ) = Settings::Find( "testing/tests" ).value_or( "*" );
	   result = RUN_ALL_TESTS();
		Process::Shutdown( result );
	}
	return result;
}