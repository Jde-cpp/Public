#include "gtest/gtest.h"
#include <jde/framework/process.h>
#include <jde/framework/settings.h>
#include <jde/access/access.h>
#include <jde/db/db.h>
#include <jde/ql/ql.h>
#include <jde/ql/QLHook.h>
#include "globals.h"

#define let const auto

namespace Jde{
	α OSApp::ProductName()ι->sv{ return "Tests.Access"; }
	up<exception> _exception;
 	α Startup( int argc, char **argv, bool& set )ε->Access::ConfigureAwait::Task{
		try{
	#ifdef _MSC_VER
			ASSERT( Settings::Get<uint>("/workers/drive/threads")>0 )
	#endif
			ASSERT( argc>1 && string{argv[1]}=="-c" )
			OSApp::Startup( argc, argv, OSApp::ProductName(), "Access tests", true );

			let metaDataName{ "access" };
			sp<Access::IAcl> authorize = Access::LocalAcl();
			auto schema = DB::GetAppSchema( metaDataName, authorize );
			QL::Configure( {schema} );//data uses ql.
			if( Settings::FindBool("/testing/recreateDB").value_or(false) )
				DB::NonProd::Recreate( *schema, QL::Local() );
			else if( Settings::FindBool("/dbServers/sync").value_or(false) )
				DB::SyncSchema( *schema, QL::Local() );
			auto await = Access::Configure( schema, {schema}, QL::Local(), UserPK{UserPK::System} );
			co_await await;

			Access::Tests::SetSchema( schema );
		}
		catch( exception& e ){//don't want unhandeled exception routine.
			_exception = ToUP( move(e) );
		}
		set = true;
	}
}

α main( int argc, char **argv )->int{
	using namespace Jde;
	::testing::InitGoogleTest( &argc, argv );
	bool set{};
	Startup( argc, argv, set );
	while( !set )
		std::this_thread::yield();
	int result = EXIT_FAILURE;
	if( _exception ){
		std::cerr << _exception->what() << std::endl;
		_exception = nullptr; //logging at finalize doesn't work.
	}
	else{
		::testing::GTEST_FLAG( filter ) = Settings::FindSV( "/testing/tests" ).value_or( "*" );
		result = RUN_ALL_TESTS();
	}
	Process::Shutdown( result );

	return result;
}