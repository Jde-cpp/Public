#include "gtest/gtest.h"
#include <jde/fwk/process/process.h>
#include <jde/fwk/settings.h>
#include <jde/fwk/co/Timer.h>
#include <jde/ql/ql.h>
#include <jde/access/Authorize.h> //!
#include <jde/access/server/accessServer.h>
#include <jde/access/AccessListener.h>
#include <jde/db/db.h>
#include "globals.h"

#define let const auto

namespace Jde{
	sp<Access::AccessListener> _listener;
	α Process::ProductName()ι->sv{ return "Tests.Access"; }
	Ω keepExecuterAlive()ι->VoidTask{
		co_await DurationTimer{ 360s };
	}
 	α Startup( int argc, char **argv )ε->void{
#ifdef _MSC_VER
		ASSERT( Settings::FindNumber<uint>("/workers/drive/threadSize").value_or(5)>0 )
#endif
		Process::Startup( argc, argv, Process::ProductName(), "Access tests", true );
		Logging::Init();
		let metaDataName{ "access" };
		auto authorizer = Access::Tests::Authorizer();
		auto schema = DB::GetAppSchema( metaDataName, authorizer );
		auto ql = QL::Configure( {schema}, authorizer );
		_listener = ms<Access::AccessListener>( ql );
		keepExecuterAlive();
		if( Settings::FindBool("/testing/recreateDB").value_or(false) )
			DB::NonProd::Recreate( *schema, ql );
		else if( Settings::FindBool("/dbServers/sync").value_or(false) )
			DB::SyncSchema( *schema, ql );
		auto await = Access::Server::Configure( {schema}, ql, UserPK{UserPK::System}, authorizer, _listener );
		BlockVoidAwait( move(await) );
		Access::Tests::SetQL( ql );
	}
}

α main( int argc, char **argv )->int{
	using namespace Jde;
	::testing::InitGoogleTest( &argc, argv );
	int exitCode{ EXIT_FAILURE };
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
	INFOT( ELogTags::App, "Shutting down with exit code {}.", exitCode );
	Process::Shutdown( exitCode );
	std::cout << "Exited with code: " << exitCode << std::endl;
	return exitCode;
}