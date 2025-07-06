#include "gtest/gtest.h"
#include <jde/framework/process.h>
#include <jde/framework/settings.h>
#include <jde/framework/coroutine/Timer.h>
#include <jde/ql/ql.h>
#include <jde/ql/QLHook.h>
#include <jde/access/Authorize.h>
#include <jde/access/server/accessServer.h>
#include <jde/access/awaits/ConfigureAwait.h>
#include <jde/db/db.h>
#include "globals.h"

#define let const auto

namespace Jde{
	α Process::ProductName()ι->sv{ return "Tests.Access"; }
	Ω keepExecuterAlive()ι->VoidAwait<>::Task{
		co_await DurationTimer{ 360s };
	}
 	α Startup( int argc, char **argv )ε->void{
#ifdef _MSC_VER
		ASSERT( Settings::FindNumber<uint>("/workers/drive/threadSize").value_or(5)>0 )
#endif
		OSApp::Startup( argc, argv, Process::ProductName(), "Access tests", true );

		let metaDataName{ "access" };
		auto authorizer = Access::Tests::Authorizer();
		auto schema = DB::GetAppSchema( metaDataName, authorizer );
		auto ql = QL::Configure( {schema}, authorizer );
		keepExecuterAlive();
		if( Settings::FindBool("/testing/recreateDB").value_or(false) )
			DB::NonProd::Recreate( *schema, ql );
		else if( Settings::FindBool("/dbServers/sync").value_or(false) )
			DB::SyncSchema( *schema, ql );
		auto await = Access::Server::Configure( {schema}, ql, UserPK{UserPK::System}, authorizer );
		BlockVoidAwait<Access::ConfigureAwait>( move(await) );
		Access::Tests::SetQL( ql );
	}
}

α main( int argc, char **argv )->int{
	using namespace Jde;
	::testing::InitGoogleTest( &argc, argv );
	int exitCode;
	try{
		Startup( argc, argv );
		::testing::GTEST_FLAG( filter ) = Settings::FindSV( "/testing/tests" ).value_or( "*" );
		exitCode = RUN_ALL_TESTS();
	}
	catch( exception& e ){
		if( auto p = dynamic_cast<IException*>(&e); p ){
			p->Log();
			exitCode = p->Code ? (int)p->Code : EXIT_FAILURE;
		}
		std::cerr << e.what() << std::endl;
	}
	Process::Shutdown( exitCode );
	return exitCode;
}