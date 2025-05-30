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
	up<exception> _error;
 	α Startup( int argc, char **argv, bool& set )ε->Access::ConfigureAwait::Task{
		try{
	#ifdef _MSC_VER
			ASSERT( Settings::FindNumber<uint>("/workers/drive/threadSize").value_or(5)>0 )
	#endif
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
			_error = ToUP( move(e) );
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
	if( _error ){
		std::cerr << _error->what() << std::endl;
		_error = nullptr; //logging at finalize doesn't work.
	}
	else{
		::testing::GTEST_FLAG( filter ) = Settings::FindSV( "/testing/tests" ).value_or( "*" );
		result = RUN_ALL_TESTS();
	}
	Process::Shutdown( result );

	return result;
}